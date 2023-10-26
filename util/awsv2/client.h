// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#pragma once

#include <string>

#include "util/awsv2/aws.h"
#include "util/awsv2/credentials_provider.h"
#include "util/awsv2/v4_signer.h"
#include "util/http/http_clientv2.h"

namespace util {
namespace awsv2 {

// Client is a wrapper for the HTTP client that handles AWS authentication and
// request retries.
class Client {
 public:
  Client(const std::string& endpoint, bool https, bool ec2_metadata, bool sign_payload);

  virtual ~Client() = default;

  AwsResult<http::Response> Send(http::Request req);

 protected:
  std::string endpoint_;

 private:
  bool https_;

  http::ClientV2 client_;

  V4Signer signer_;

  std::unique_ptr<CredentialsProvider> credentials_provider_;
};

}  // namespace awsv2
}  // namespace util
