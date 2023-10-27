// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#pragma once

#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/url/url.hpp>
#include <map>
#include <string>

namespace util {
namespace http {

namespace h2 = boost::beast::http;

struct Request {
  h2::verb method;
  boost::urls::url url;
  std::map<std::string, std::string> headers;
  std::string_view body;
};

struct Response {
  h2::status status;
  std::string body;
};

}  // namespace http
}  // namespace util
