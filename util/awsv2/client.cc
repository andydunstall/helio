// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/awsv2/client.h"

namespace util {
namespace awsv2 {

Client::Client(const std::string& endpoint, bool https, bool ec2_metadata, bool sign_payload) {
}

AwsResult<http::Response> Client::Send(const http::Request& req) {
  return {};
}

}  // namespace awsv2
}  // namespace util
