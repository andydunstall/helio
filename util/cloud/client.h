// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#pragma once

#include <boost/beast/http/message.hpp>
#include <pugixml.hpp>
#include <system_error>

#include "base/logging.h"
#include "util/cloud/aws.h"
#include "util/fibers/proactor_base.h"
#include "util/http/http_client.h"

namespace util {
namespace cloud {

namespace h2 = boost::beast::http;

class Client {
 public:
  Client(const AWS* aws, http::Client* http_client);

  template <typename Req, typename Resp> std::error_code Request(Req* req, Resp* resp);

 private:
  template <typename Req> std::error_code ConnectIfNeeded(Req* req);

  template <typename Resp> io::Result<std::string> ParseErrorCode(Resp* resp) const;

  void RetryBackoff(int attempt) const;

  const AWS* aws_;

  http::Client* client_;

  AwsSignKey sign_key_;

  bool reconnect_ = false;
};

template <typename Req, typename Resp> std::error_code Client::Request(Req* req, Resp* resp) {
  std::error_code ec;

  int attempt = 0;
  while (true) {
    attempt++;

    RetryBackoff(attempt);

    if (attempt > 5) {
      return ec;
    }

    ec = ConnectIfNeeded(req);
    if (ec) {
      continue;
    }

    // Resign on each retry since the credentials may have been updated.
    // TODO(andydunstall) Only support empty or unsigned payload.
    boost::optional<std::uint64_t> payload_size = req->payload_size();
    if (payload_size && *payload_size == 0) {
      sign_key_.Sign(AWS::kEmptySig, req);
    } else {
      sign_key_.Sign(AWS::kUnsignedPayloadSig, req);
    }

    absl::Time attempt_time = absl::Now();
    DVLOG(3) << "aws client: send request: (attempt = " << attempt << ", "
             << "attempt_time = " << attempt_time << ")" << std::endl
             << "-------------------------" << std::endl
             << *req << "-------------------------";

    ec = client_->Send(*req, resp);
    if (ec) {
      DVLOG(1) << "aws client: failed to send request; error=" << ec;
      // If we failed to send the request we reconnect.
      // TODO(andydunstall) AWS::SendRequest only reconnects if it gets
      // boost::system::errc::connection_aborted.
      reconnect_ = true;
      continue;
    }

    DVLOG(3) << "aws client: recv response: " << std::endl
             << "-------------------------" << std::endl
             << *resp << "-------------------------";

    auto& headers = resp->base();
    if (headers[h2::field::connection] == "close") {
      reconnect_ = true;
    }

    // TODO(andydunstall) Incorrectly getting OK after a disconnect (with no
    // body)
    if (resp->result() == h2::status::ok) {
      return std::error_code{};
    }

    io::Result<std::string> error_code = ParseErrorCode(resp);
    if (!error_code) {
      // If we can't parse the error, retry anyway.
      LOG(WARNING) << "aws client: failed to parse error code in non-200 response";
      continue;
    }

    if (*error_code == "ExpiredToken" || *error_code == "ExpiredTokenException") {
      VLOG(1) << "aws client: expired credentials; refreshing credentials";
      aws_->RefreshToken();
      sign_key_ = aws_->GetSignKey(aws_->connection_data().region);
      continue;
    }

    // TODO(andydunstall) Handle other errors...
  }

  return std::error_code{};
}

template <typename Req> std::error_code Client::ConnectIfNeeded(Req* req) {
  if (!reconnect_) {
    return std::error_code{};
  }

  auto host_it = req->find(h2::field::host);
  if (host_it == req->end()) {
    LOG(WARNING) << "aws client: request: missing host header";
    return std::make_error_code(std::errc::invalid_argument);
  }

  std::string host = host_it->value();
  std::string port;

  size_t pos = host.rfind(':');
  if (pos != std::string::npos) {
    port = host.substr(pos + 1);
    host = host.substr(0, pos);
  } else {
    port = "80";
  }

  DVLOG(1) << "aws client: reconnecting; host=" << host << "; port=" << port;

  std::error_code ec = client_->Connect(host, port);
  if (ec) {
    LOG(WARNING) << "aws client: request: failed to connect; host=" << host << "; error=" << ec;
    return ec;
  }

  reconnect_ = false;

  return std::error_code{};
}

template <typename Resp> io::Result<std::string> Client::ParseErrorCode(Resp* resp) const {
  // We only support S3 so assume XML.
  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load_buffer(resp->body().data(), resp->body().size());
  if (!result) {
    LOG(ERROR) << "aws client: failed to parse xml response: " << result.description();
    return nonstd::make_unexpected(std::make_error_code(std::errc::bad_message));
  }
  pugi::xml_node root = doc.child("Error");
  if (root.type() != pugi::node_element) {
    LOG(ERROR) << "aws client: failed to parse error code";
    return nonstd::make_unexpected(std::make_error_code(std::errc::bad_message));
  }
  std::string code = root.child("Code").text().get();
  if (code.empty()) {
    LOG(ERROR) << "aws client: failed to parse error code";
    return nonstd::make_unexpected(std::make_error_code(std::errc::bad_message));
  }
  return code;
}

}  // namespace cloud
}  // namespace util
