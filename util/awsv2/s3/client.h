// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#pragma once

#include "util/awsv2/aws.h"
#include "util/awsv2/client.h"

namespace util {
namespace awsv2 {
namespace s3 {

struct ListBucketsResult {
  std::vector<std::string> buckets;

  static AwsResult<ListBucketsResult> Parse(std::string_view s);
};

class Client : public awsv2::Client {
 public:
  Client(const std::string& region);

  AwsResult<std::vector<std::string>> ListBuckets();
};

}  // namespace s3
}  // namespace awsv2
}  // namespace util
