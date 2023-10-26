// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#pragma once

#include <absl/types/span.h>

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/url/url.hpp>
#include <map>
#include <memory>

#include "io/io.h"
#include "util/fiber_socket_base.h"
#include "util/fibers/proactor_base.h"

namespace util {
namespace http {

namespace h2 = boost::beast::http;

struct Request {
  h2::verb method;
  boost::urls::url url;
  std::map<std::string, std::string> headers;
  absl::Span<uint8_t> body;
};

struct Response {
  h2::status status;
  std::string body;
};

enum class HttpError {
  RESOLVE,
  CONNECT,
  NETWORK,
};

template <typename T> using HttpResult = io::Result<T, HttpError>;

class ClientV2 {
 public:
  ClientV2();

  HttpResult<Response> Send(const Request& req);

 private:
  HttpResult<std::unique_ptr<FiberSocketBase>> Connect(std::string_view host, uint16_t port,
                                                       bool tls);

  HttpResult<boost::asio::ip::address> Resolve(std::string_view host) const;

  ProactorBase* proactor_;

  boost::beast::flat_buffer buf_;
};

}  // namespace http
}  // namespace util
