// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/awsv2/client.h"

namespace util {
namespace awsv2 {

Client::Client(std::unique_ptr<CredentialsProvider> credentials_provider)
    : credentials_provider_{std::move(credentials_provider)} {
}

AwsResult<Response> Client::Send(Request req) {
  return SendAttempt(req);
}

AwsResult<Response> Client::SendAttempt(Request req) {
  std::optional<Credentials> creds = credentials_provider_->LoadCredentials();
  if (!creds) {
    return nonstd::make_unexpected(AwsError{AwsErrorType::UNAUTHORIZED, "credentials not found"});
  }

  Response resp;
  return resp;
}

}  // namespace awsv2
}  // namespace util
