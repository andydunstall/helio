// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/http/http_clientv2.h"

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/write.hpp>

#include "base/logging.h"
#include "util/asio_stream_adapter.h"
#include "util/fibers/dns_resolve.h"

namespace util {
namespace http {

namespace {

constexpr uint16_t kHttpPort = 80;
constexpr uint16_t kHttpsPort = 443;

constexpr unsigned kHttpVersion1_1 = 11;

}  // namespace

ClientV2::ClientV2() {
  proactor_ = ProactorBase::me();
}

HttpResult<Response> ClientV2::Send(const Request& req) {
  VLOG(1) << "http client: send request; method=" << h2::to_string(req.method)
          << "; url=" << req.url.buffer();

  uint16_t port = req.url.port_number();
  if (port == 0) {
    if (req.url.scheme() == "http") {
      port = kHttpPort;
    }
    if (req.url.scheme() == "https") {
      port = kHttpsPort;
    }
  }

  h2::request<h2::string_body> http_req{req.method, req.url.path(), kHttpVersion1_1};
  for (const auto& header : req.headers) {
    http_req.set(header.first, header.second);
  }

  HttpResult<std::unique_ptr<FiberSocketBase>> connect_res =
      Connect(req.url.host(), port, req.url.scheme() == "https");
  if (!connect_res) {
    return nonstd::make_unexpected(connect_res.error());
  }

  std::unique_ptr<FiberSocketBase> conn = std::move(*connect_res);

  boost::system::error_code bec;
  AsioStreamAdapter<> adapter(*conn);
  h2::write(adapter, http_req, bec);
  if (bec) {
    LOG(WARNING) << "http client: failed to send request; method=" << h2::to_string(req.method)
                 << "; url=" << req.url.buffer();
    conn->Close();
    return nonstd::make_unexpected(HttpError::NETWORK);
  }

  // TODO ideally move the underlying string to avoid being freed
  h2::response<h2::string_body> http_resp;
  h2::read(adapter, buf_, http_resp, bec);
  if (bec) {
    LOG(WARNING) << "http client: failed to read response; method=" << h2::to_string(req.method)
                 << "; url=" << req.url.buffer();
    conn->Close();
    return nonstd::make_unexpected(HttpError::NETWORK);
  }

  conn->Close();

  Response resp{};
  resp.status = http_resp.result();
  resp.body = std::move(http_resp.body());

  VLOG(1) << "http client: received response; status=" << resp.status;

  return resp;
}

HttpResult<std::unique_ptr<FiberSocketBase>> ClientV2::Connect(std::string_view host, uint16_t port,
                                                               bool tls) {
  // TODO(andydunstall): TLS support.
  // TODO(andydunstall): Cache connection. Clear connection on an error.

  VLOG(2) << "http client: connect; host=" << host << "; port=" << port
          << "; tls=" << std::boolalpha << tls;

  HttpResult<boost::asio::ip::address> addr = Resolve(host);
  if (!addr) {
    return nonstd::make_unexpected(addr.error());
  }

  std::unique_ptr<FiberSocketBase> socket;
  socket.reset(proactor_->CreateSocket());
  FiberSocketBase::endpoint_type ep{*addr, port};
  std::error_code ec = socket->Connect(ep);
  if (ec) {
    LOG(WARNING) << "http client: failed to connect; addr=" << *addr;
    socket->Close();
    return nonstd::make_unexpected(HttpError::CONNECT);
  }

  VLOG(1) << "http client: connected; host=" << host << "; port=" << port;

  // TODO(andydunstall) Enable keepalives.

  return socket;
}

HttpResult<boost::asio::ip::address> ClientV2::Resolve(std::string_view host) const {
  VLOG(2) << "http client: resolving host; host=" << host;

  char ip[INET_ADDRSTRLEN];
  // TODO(andydunstall): Configure timeout
  std::error_code ec = fb2::DnsResolve(host.data(), 5000, ip, proactor_);
  if (ec) {
    LOG(WARNING) << "http client: failed to resolve host; host=" << host;
    return nonstd::make_unexpected(HttpError::RESOLVE);
  }

  boost::system::error_code bec;
  boost::asio::ip::address address = boost::asio::ip::make_address(ip, bec);
  if (bec) {
    LOG(ERROR) << "http client: resolved invalid address";
    return nonstd::make_unexpected(HttpError::RESOLVE);
  }

  VLOG(2) << "http client: resolved host; host=" << host << "; ip=" << address;

  return address;
}

}  // namespace http
}  // namespace util
