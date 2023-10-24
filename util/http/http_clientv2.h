// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#pragma once

#include <absl/types/span.h>

#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/url/url.hpp>

#include "io/io.h"

namespace util {
namespace http {

namespace h2 = boost::beast::http;

struct Request {
  h2::verb method;
  boost::urls::url url;
  absl::Span<uint8_t> body;
};

struct Response {
  h2::status status;
  absl::Span<uint8_t> body;
};

enum class HttpError {
  RESOLVE_ERR,
  CONNECT_ERR,
  NETWORK_ERR,
};

template <typename T> using HttpResult = io::Result<T, HttpError>;

class ClientV2 {
 public:
  HttpResult<Response> Send(const Request& req);
};

}  // namespace http
}  // namespace util
