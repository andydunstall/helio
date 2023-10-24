// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/awsv2/client.h"

#include "base/logging.h"

namespace util {
namespace awsv2 {

Client::Client(const std::string& endpoint, bool https, bool ec2_metadata, bool sign_payload) {
  credentials_provider_ = std::make_unique<EnvironmentCredentialsProvider>();
}

AwsResult<http::Response> Client::Send(const http::Request& req) {
  // TODO(andydunstall): Set scheme and host.

  // TODO(andydunstall): Set authorization header.
  // TODO(andydunstall): Handle errors and retries.

  std::optional<Credentials> creds = credentials_provider_->GetCredentials();
  if (!creds) {
    LOG(ERROR) << "aws: failed to load credentials";
    return nonstd::make_unexpected(AwsError::UNAUTHORIZED);
  }

  client_.Send(req);

  return {};
}

}  // namespace awsv2
}  // namespace util
