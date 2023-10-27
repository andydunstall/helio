// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/awsv2/client.h"

#include "base/logging.h"

namespace util {
namespace awsv2 {

Client::Client(const std::string& region, const std::string& service)
    : client_{}, signer_{region, service} {
  credentials_provider_ = std::make_unique<EnvironmentCredentialsProvider>();
}

AwsResult<std::string> Client::Send(Request* req) {
  std::optional<Credentials> creds = credentials_provider_->LoadCredentials();
  if (!creds) {
    LOG(ERROR) << "aws: failed to load credentials";
    return nonstd::make_unexpected(AwsError::UNAUTHORIZED);
  }

  signer_.SignRequest(*creds, req);

  HttpResult<Response> resp = client_.Send(*req);
  if (!resp) {
    // TODO
  }

  if (h2::to_status_class(resp->status) != h2::status_class::successful) {
    // TODO
  }

  return resp->body;
}

}  // namespace awsv2
}  // namespace util
