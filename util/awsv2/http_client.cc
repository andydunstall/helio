// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/awsv2/http_client.h"

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/write.hpp>

#include "base/logging.h"
#include "util/asio_stream_adapter.h"
#include "util/fibers/dns_resolve.h"
#include "util/http/http_client.h"

namespace util {
namespace awsv2 {

namespace {

constexpr uint16_t kHttpPort = 80;
constexpr uint16_t kHttpsPort = 443;

constexpr unsigned kHttpVersion1_1 = 11;

}  // namespace

HttpClient::HttpClient() {
  proactor_ = ProactorBase::me();
  ctx_ = http::TlsClient::CreateSslContext();
}

HttpClient::~HttpClient() {
  http::TlsClient::FreeContext(ctx_);
}

HttpResult<Response> HttpClient::Send(const Request& req) {
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

  if (!Connect(req.url.host(), port, req.url.scheme() == "https")) {
    return nonstd::make_unexpected(HttpError::CONNECT);
  }

  const std::string path = (req.url.path().empty()) ? "/" : req.url.path();
  h2::request<h2::string_body> http_req{req.method, path, kHttpVersion1_1};
  for (const auto& header : req.headers) {
    http_req.set(header.first, header.second);
  }
  // TODO(andydunstall): Support body.

  h2::response<h2::string_body> http_resp;
  std::error_code ec = conn_->client->Send(http_req, &http_resp);
  if (ec) {
    LOG(WARNING) << "http client: failed to send request; method=" << h2::to_string(req.method)
                 << "; url=" << req.url.buffer();

    // Discard the cached connection.
    conn_ = std::nullopt;

    return nonstd::make_unexpected(HttpError::REQUEST);
  }

  Response resp{};
  resp.status = http_resp.result();
  resp.body = std::move(http_resp.body());

  VLOG(1) << "http client: received response; status=" << resp.status;

  return resp;
}

bool HttpClient::Connect(std::string_view host, uint16_t port, bool tls) {
  if (conn_) {
    if (conn_->host == host && conn_->port == port && conn_->tls == tls) {
      return true;
    }

    // Discard the cached connection.
    conn_ = std::nullopt;
  }

  HttpClient::Connection conn;
  conn.host = std::string(host);
  conn.port = port;
  conn.tls = tls;

  if (tls) {
    std::unique_ptr<http::TlsClient> client = std::make_unique<http::TlsClient>(proactor_);
    std::error_code ec = client->Connect(host, absl::StrFormat("%d", port), ctx_);
    if (ec) {
      LOG(WARNING) << "http client: failed to connect to host; host=" << host << "; port=" << port;
      return false;
    }

    conn.client = std::move(client);
  } else {
    std::unique_ptr<http::Client> client = std::make_unique<http::Client>(proactor_);
    std::error_code ec = client->Connect(host, absl::StrFormat("%d", port));
    if (ec) {
      LOG(WARNING) << "http client: failed to connect to host; host=" << host << "; port=" << port;
      return false;
    }

    conn.client = std::move(client);
  }

  conn_ = std::move(conn);

  return true;
}

}  // namespace awsv2
}  // namespace util
