// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/awsv2/client.h"

#include "base/logging.h"

namespace util {
namespace awsv2 {

Client::Client(const std::string& endpoint, bool https, bool ec2_metadata, bool sign_payload)
    : endpoint_{endpoint}, https_{https}, signer_{"us-east-1", "s3", sign_payload} {
  credentials_provider_ = std::make_unique<EnvironmentCredentialsProvider>();
}

AwsResult<http::Response> Client::Send(http::Request req) {
  // TODO(andydunstall): Set authorization header.
  // TODO(andydunstall): Handle errors and retries.

  std::optional<Credentials> creds = credentials_provider_->GetCredentials();
  if (!creds) {
    LOG(ERROR) << "aws: failed to load credentials";
    return nonstd::make_unexpected(AwsError::UNAUTHORIZED);
  }

  if (https_) {
    req.url.set_scheme("https");
  } else {
    req.url.set_scheme("http");
  }
  // TODO(andydunstall): Support endpoint with a port.
  const std::string host = (endpoint_.empty()) ? "s3.amazonaws.com" : endpoint_;
  req.url.set_host(host);
  req.headers.emplace("host", host);

  signer_.SignRequest(*creds, &req);

  http::HttpResult<http::Response> resp = client_.Send(req);
  if (!resp) {
    // TODO
  }

  // TODO if not 2xx ...

  return *resp;
}

}  // namespace awsv2
}  // namespace util
