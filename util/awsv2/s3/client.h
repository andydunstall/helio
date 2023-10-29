// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#pragma once

#include <string>
#include <vector>

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
  Client(const Config& config,
         std::unique_ptr<util::awsv2::CredentialsProvider> credentials_provider);

  // Lists all the buckets owned by this account.
  AwsResult<std::vector<std::string>> ListBuckets();
};

}  // namespace s3
}  // namespace awsv2
}  // namespace util
