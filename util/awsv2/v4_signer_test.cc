// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/awsv2/v4_signer.h"

#include <absl/time/clock.h>

#include "base/gtest.h"

namespace util {
namespace awsv2 {

namespace h2 = boost::beast::http;

class V4SignerTest : public ::testing::Test {};

TEST_F(V4SignerTest, SignHttps) {
  Credentials creds;
  creds.access_key_id = "key";
  creds.secret_access_key = "secret";

  Request req;
  req.method = h2::verb::get;
  req.url.SetHost("s3.amazonaws.com");
  req.url.SetPath("/foo");
  req.headers = std::map<std::string, std::string>{
      {"host", "s3.amazonaws.com"},
  };
  req.body = "myrequest";

  V4Signer signer{"eu-west-2", "s3"};
  signer.SignRequest(creds, &req, absl::FromUnixSeconds(1'000'000'000));

  EXPECT_EQ("20010909T014600Z", req.headers["x-amz-date"]);
  EXPECT_EQ("UNSIGNED-PAYLOAD", req.headers["x-amz-content-sha256"]);
  EXPECT_EQ(
      "AWS4-HMAC-SHA256 Credential=key/20010909/eu-west-2/s3/aws4_request, "
      "SignedHeaders=host;x-amz-content-sha256;x-amz-date, "
      "Signature=5a0782d56b363dfb659b624205cb5c4a6f989ab10fa21072eee54985bfb3bacd",
      req.headers["authorization"]);
}

// Sign a HTTP request to which signs the payload hash SHA256.
TEST_F(V4SignerTest, SignHttp) {
  Credentials creds;
  creds.access_key_id = "key";
  creds.secret_access_key = "secret";

  Request req;
  req.method = h2::verb::get;
  req.url.SetScheme(Scheme::HTTP);
  req.url.SetHost("s3.amazonaws.com");
  req.url.SetPath("/foo");
  req.headers = std::map<std::string, std::string>{
      {"host", "s3.amazonaws.com"},
  };
  req.body = "myrequest";

  V4Signer signer{"eu-west-2", "s3"};
  signer.SignRequest(creds, &req, absl::FromUnixSeconds(1'000'000'000));

  EXPECT_EQ("20010909T014600Z", req.headers["x-amz-date"]);
  EXPECT_EQ("2306e32eaaa1ead1d9a8938f273af7e2ef0c069384220483480e26e04c497658",
            req.headers["x-amz-content-sha256"]);
  EXPECT_EQ(
      "AWS4-HMAC-SHA256 Credential=key/20010909/eu-west-2/s3/aws4_request, "
      "SignedHeaders=host;x-amz-checksum-sha256;x-amz-content-sha256;x-amz-date, "
      "Signature=ca353be64569d89d678eed53c67583dc3e2dc49e5a0d219c4519d1417f655f42",
      req.headers["authorization"]);
}

TEST_F(V4SignerTest, SignWithSecurityToken) {
  Credentials creds;
  creds.access_key_id = "key";
  creds.secret_access_key = "secret";
  creds.session_token = "token";

  Request req;
  req.method = h2::verb::get;
  req.url.SetHost("s3.amazonaws.com");
  req.url.SetPath("/foo");
  req.headers = std::map<std::string, std::string>{
      {"host", "s3.amazonaws.com"},
  };
  req.body = "myrequest";

  V4Signer signer{"eu-west-2", "s3"};
  signer.SignRequest(creds, &req, absl::FromUnixSeconds(1'000'000'000));

  EXPECT_EQ(creds.session_token, req.headers["x-amz-security-token"]);
  EXPECT_EQ("20010909T014600Z", req.headers["x-amz-date"]);
  EXPECT_EQ("UNSIGNED-PAYLOAD", req.headers["x-amz-content-sha256"]);
  EXPECT_EQ(
      "AWS4-HMAC-SHA256 Credential=key/20010909/eu-west-2/s3/aws4_request, "
      "SignedHeaders=host;x-amz-content-sha256;x-amz-date;x-amz-security-token, "
      "Signature=d82c0671bdf355f421a87acf7b24acd8dd49e1f31256c71baf74e2549635e7ed",
      req.headers["authorization"]);
}

}  // namespace awsv2
}  // namespace util
