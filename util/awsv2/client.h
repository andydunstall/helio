// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#pragma once

#include "util/awsv2/aws.h"
#include "util/awsv2/credentials_provider.h"

namespace util {
namespace awsv2 {

class Client {
 public:
  Client(std::unique_ptr<CredentialsProvider> credentials_provider);

  AwsResult<Response> Send(Request req);

 private:
  AwsResult<Response> SendAttempt(Request req);

  std::unique_ptr<CredentialsProvider> credentials_provider_;
};

}  // namespace awsv2
}  // namespace util
