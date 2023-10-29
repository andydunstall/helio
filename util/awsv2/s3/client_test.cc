// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/awsv2/s3/client.h"

#include "base/gtest.h"

namespace util {
namespace awsv2 {
namespace s3 {

class ListBucketsResultTest : public ::testing::Test {};

TEST_F(ListBucketsResultTest, ParseOK) {
  AwsResult<ListBucketsResult> result =
      ListBucketsResult::Parse(R"(<?xml version="1.0" encoding="UTF-8"?>
<ListAllMyBucketsResult xmlns="http://s3.amazonaws.com/doc/2006-03-01/">
  <Owner>
    <ID>69c83fe29af47fc84d19820e7c8e1d691939acc68155d729d186420856da486c</ID>
    <DisplayName>aws-notify+dev</DisplayName>
  </Owner>
  <Buckets>
    <Bucket>
      <Name>bucket-1</Name>
      <CreationDate>2023-08-31T04:59:27.000Z</CreationDate>
    </Bucket>
    <Bucket>
      <Name>bucket-2</Name>
      <CreationDate>2023-10-31T09:32:11.000Z</CreationDate>
    </Bucket>
  </Buckets>
</ListAllMyBucketsResult>
)");
  EXPECT_TRUE(result);
  std::vector<std::string> expected_buckets{"bucket-1", "bucket-2"};
  EXPECT_EQ(expected_buckets, result->buckets);
}

TEST_F(ListBucketsResultTest, ParseInvalidXml) {
  AwsResult<ListBucketsResult> result = ListBucketsResult::Parse("invalid xml...");
  EXPECT_FALSE(result);
  EXPECT_EQ(AwsErrorType::INVALID_RESPONSE, result.error().type);
}

}  // namespace s3
}  // namespace awsv2
}  // namespace util
