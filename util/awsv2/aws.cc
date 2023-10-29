// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/awsv2/aws.h"

#include <pugixml.hpp>
#include <sstream>

#include "base/logging.h"

namespace util {
namespace awsv2 {

namespace {

AwsErrorType CodeToErrorType(std::string_view s) {
  if (s == "InvalidToken") {
    return AwsErrorType::INVALID_TOKEN;
  }
  // TODO(andydunstall): See CoreErrors.cpp
  return AwsErrorType::UNKNOWN;
}

}  // namespace

std::string ToString(AwsErrorType type) {
  switch (type) {
    case AwsErrorType::UNAUTHORIZED:
      return "unauthorized";
    case AwsErrorType::NETWORK:
      return "network";
    case AwsErrorType::INVALID_RESPONSE:
      return "invalid_response";
    case AwsErrorType::ACCESS_DENIED:
      return "access_denied";
    case AwsErrorType::INVALID_TOKEN:
      return "invalid_token";
    case AwsErrorType::RESOURCE_NOT_FOUND:
      return "resource_not_found";
    default:
      return "unknown";
  }
}

AwsError::AwsError(AwsErrorType type_, std::string message_, bool retryable_)
    : type{type_}, message{message_}, retryable{retryable_} {
}

std::string AwsError::ToString() const {
  std::stringstream ss;
  ss << awsv2::ToString(type) << ": " << message;
  return ss.str();
}

AwsError AwsError::Parse(std::string_view s) {
  pugi::xml_document doc;
  const pugi::xml_parse_result xml_result = doc.load_buffer(s.data(), s.size());
  if (!xml_result) {
    return AwsError{AwsErrorType::INVALID_RESPONSE, "parse error response: invalid xml", false};
  }

  const pugi::xml_node root = doc.child("Error");
  if (root.type() != pugi::node_element) {
    return AwsError{AwsErrorType::INVALID_RESPONSE, "parse error response: Error not found", false};
  }

  const std::string code = root.child("Code").text().get();
  if (code.empty()) {
    return AwsError{AwsErrorType::INVALID_RESPONSE, "parse error response: Code not found", false};
  }

  const std::string message = root.child("Message").text().get();
  return AwsError{CodeToErrorType(code), message, false};
}

}  // namespace awsv2
}  // namespace util
