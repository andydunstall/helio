// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#pragma once

#include <string>

#include "util/awsv2/aws.h"
#include "util/awsv2/credentials_provider.h"
#include "util/awsv2/http_client.h"
#include "util/awsv2/v4_signer.h"

namespace util {
namespace awsv2 {

class Client {
 public:
  Client(const std::string& region, const std::string& service);

  AwsResult<std::string> Send(Request* req);

 private:
  HttpClient client_;

  V4Signer signer_;

  std::unique_ptr<CredentialsProvider> credentials_provider_;
};

}  // namespace awsv2
}  // namespace util
