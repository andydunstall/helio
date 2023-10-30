// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#pragma once

#include "util/awsv2/aws.h"
#include "util/awsv2/credentials_provider.h"
#include "util/awsv2/http_client.h"
#include "util/awsv2/v4_signer.h"

namespace util {
namespace awsv2 {

struct Config {
  std::string region;
  std::string endpoint;
  bool https;
};

class Client {
 public:
  Client(const Config& config, std::unique_ptr<CredentialsProvider> credentials_provider,
         const std::string& service);

  AwsResult<Response> Send(Request req);

 protected:
  Config config_;

 private:
  AwsResult<Response> SendAttempt(Request req);

  std::unique_ptr<CredentialsProvider> credentials_provider_;

  V4Signer signer_;

  HttpClient client_;
};

}  // namespace awsv2
}  // namespace util
