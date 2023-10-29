// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#pragma once

#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <map>
#include <ostream>
#include <string>

#include "io/io.h"
#include "util/awsv2/url.h"

namespace util {
namespace awsv2 {

namespace h2 = boost::beast::http;

struct Request {
  h2::verb method;
  Url url;
  std::map<std::string, std::string> headers;
  std::string_view body;
};

struct Response {
  h2::status status;
  std::string body;
  std::map<std::string, std::string> headers;
};

enum class AwsErrorType {
  UNAUTHORIZED,
  NETWORK,
  INVALID_RESPONSE,
  ACCESS_DENIED,
  INVALID_TOKEN,
  RESOURCE_NOT_FOUND,
  UNKNOWN,
};

std::string ToString(AwsErrorType type);

struct AwsError {
  AwsError(AwsErrorType type, std::string message, bool retryable);

  AwsErrorType type;
  std::string message;
  bool retryable;

  std::string ToString() const;

  static AwsError Parse(std::string_view s);
};

template <typename T> using AwsResult = io::Result<T, AwsError>;

}  // namespace awsv2
}  // namespace util
