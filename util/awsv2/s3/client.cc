// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/awsv2/s3/client.h"

namespace util {
namespace awsv2 {
namespace s3 {

AwsResult<std::vector<std::string>> Client::ListBuckets() {
  return {};
}

}  // namespace s3
}  // namespace awsv2
}  // namespace util
