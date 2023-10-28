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

struct ListObjectsResult {
  std::vector<std::string> objects;
  std::string continuation_token;

  static AwsResult<ListObjectsResult> Parse(std::string_view s);
};

struct CreateMultipartUploadResult {
  std::string upload_id;

  static AwsResult<CreateMultipartUploadResult> Parse(std::string_view s);
};

struct GetObjectResult {
  std::string body;
  size_t object_size;
};

class Client : public awsv2::Client {
 public:
  Client(const std::string& region);

  // Lists all the buckets owned by this account.
  AwsResult<std::vector<std::string>> ListBuckets();

  // Lists the objects in the bucket with the given prefix.
  //
  // Returns up to the given limit, or all objects if the limit is 0.
  AwsResult<std::vector<std::string>> ListObjects(std::string_view bucket, std::string_view prefix,
                                                  size_t limit = 0);

  AwsResult<GetObjectResult> GetObject(std::string_view bucket, std::string_view key,
                                       std::string_view range);

  AwsResult<std::string> CreateMultipartUpload(std::string_view bucket, std::string_view key);

  AwsResult<std::string> UploadPart(std::string_view bucket, std::string_view key, int part_number,
                                    std::string_view upload_id, std::string_view part);

  AwsResult<void> CompleteMultipartUpload(std::string_view bucket, std::string_view key,
                                          std::string_view upload_id,
                                          const std::vector<std::string>& parts);
};

}  // namespace s3
}  // namespace awsv2
}  // namespace util
