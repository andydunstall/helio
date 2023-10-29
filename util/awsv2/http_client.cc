// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/awsv2/http_client.h"

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

#include "base/logging.h"
#include "util/http/http_client.h"

namespace util {
namespace awsv2 {

namespace {

constexpr unsigned kHttpVersion1_1 = 11;

}  // namespace

HttpClient::HttpClient() {
  proactor_ = ProactorBase::me();
  ctx_ = http::TlsClient::CreateSslContext();
}

HttpClient::~HttpClient() {
  http::TlsClient::FreeContext(ctx_);
}

AwsResult<Response> HttpClient::Send(const Request& req) {
  std::string path = req.url.path();
  if (!req.url.QueryString().empty()) {
    path += "?";
    path += req.url.QueryString();
  }

  VLOG(1) << "http client: send request; method=" << h2::to_string(req.method)
          << "; url=" << req.url.ToString();

  if (!Connect(req.url.host(), req.url.port(), req.url.scheme() == Scheme::HTTPS)) {
    return nonstd::make_unexpected(AwsError{
        AwsErrorType::NETWORK,
        "failed to connect to host",
    });
  }

  h2::request<h2::string_body> http_req{req.method, path, kHttpVersion1_1};
  for (const auto& header : req.headers) {
    http_req.set(header.first, header.second);
  }
  http_req.body() = req.body;

  h2::response<h2::string_body> http_resp;
  std::error_code ec = conn_->client->Send(http_req, &http_resp);
  if (ec) {
    // Discard the cached connection.
    conn_ = std::nullopt;

    return nonstd::make_unexpected(AwsError{
        AwsErrorType::NETWORK,
        "failed to send http request",
    });
  }

  Response resp{};
  resp.status = http_resp.result();
  resp.body = std::move(http_resp.body());
  for (const auto& h : http_resp.base()) {
    resp.headers.emplace(std::string(h.name_string()), std::string(h.value()));
  }

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
      return false;
    }

    conn.client = std::move(client);
  } else {
    std::unique_ptr<http::Client> client = std::make_unique<http::Client>(proactor_);
    std::error_code ec = client->Connect(host, absl::StrFormat("%d", port));
    if (ec) {
      return false;
    }

    conn.client = std::move(client);
  }

  conn_ = std::move(conn);

  return true;
}

}  // namespace awsv2
}  // namespace util
