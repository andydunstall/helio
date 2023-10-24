// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/awsv2/s3/client.h"

namespace util {
namespace awsv2 {
namespace s3 {

Client::Client(const std::string& endpoint, bool https, bool ec2_metadata, bool sign_payload){};

AwsResult<std::vector<std::string>> Client::ListBuckets() {
  return std::vector<std::string>{};
}

AwsResult<std::vector<std::string>> Client::ListObjects(std::string_view bucket,
                                                        std::string_view prefix) {
  return std::vector<std::string>{};
}

}  // namespace s3
}  // namespace awsv2
}  // namespace util
