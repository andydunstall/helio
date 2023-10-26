// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#pragma once

#include <string>
#include <vector>

#include "util/awsv2/aws.h"
#include "util/awsv2/client.h"

namespace util {
namespace awsv2 {
namespace s3 {

class Client : public awsv2::Client {
 public:
  Client(const std::string& endpoint, bool https, bool ec2_metadata, bool sign_payload);

  AwsResult<std::vector<std::string>> ListBuckets();

  AwsResult<std::vector<std::string>> ListObjects(std::string_view bucket,
                                                  std::string_view prefix = "");

  AwsResult<std::string> CreateMultipartUpload(std::string_view bucket, std::string_view key);

  AwsResult<std::string> UploadPart(std::string_view bucket, std::string_view key, int part_number,
                                    std::string_view upload_id, std::string_view body);

  AwsResult<std::string> CompleteMultipartUpload(std::string_view bucket, std::string_view key,
                                                  std::string_view upload_id);
};

}  // namespace s3
}  // namespace awsv2
}  // namespace util
