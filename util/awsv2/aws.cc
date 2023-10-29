// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/awsv2/aws.h"

#include <sstream>

namespace util {
namespace awsv2 {

std::string ToString(AwsErrorType type) {
  switch (type) {
    case AwsErrorType::UNAUTHORIZED:
      return "unauthorized";
    case AwsErrorType::INVALID_RESPONSE:
      return "invalid_response";
    default:
      return "unknown";
  }
}

AwsError::AwsError(AwsErrorType type_, std::string message_) : type{type_}, message{message_} {
}

std::string AwsError::ToString() const {
  std::stringstream ss;
  ss << awsv2::ToString(type) << ": " << message;
  return ss.str();
}

}  // namespace awsv2
}  // namespace util
