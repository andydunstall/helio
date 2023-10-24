// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#pragma once

#include <optional>
#include <string>
#include <vector>

#include "util/awsv2/aws.h"
#include "util/awsv2/credentials.h"

namespace util {
namespace awsv2 {

class CredentialsProvider {
 public:
  virtual ~CredentialsProvider() = default;

  virtual std::optional<Credentials> GetCredentials() = 0;
};

// Reads AWS credentials from environment variable AWS_ACCESS_KEY_ID,
// AWS_SECRET_ACCESS_KEY and AWS_SESSION_TOKEN if they exist.
class EnvironmentCredentialsProvider : public CredentialsProvider {
 public:
  std::optional<Credentials> GetCredentials() override;
};

}  // namespace awsv2
}  // namespace util
