// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/awsv2/client.h"

namespace util {
namespace awsv2 {

namespace {

bool IsHttpStatusRetryable(h2::status status) {
  return h2::to_status_class(status) == h2::status_class::server_error;
}

AwsError HttpStatusToAwsError(h2::status status) {
  switch (status) {
    case h2::status::forbidden:
    case h2::status::unauthorized:
      return AwsError{AwsErrorType::ACCESS_DENIED, "access denied", IsHttpStatusRetryable(status)};
    case h2::status::not_found:
      return AwsError{AwsErrorType::RESOURCE_NOT_FOUND, "resource not found",
                      IsHttpStatusRetryable(status)};
    default:
      return AwsError{AwsErrorType::UNKNOWN, "unknown error", IsHttpStatusRetryable(status)};
  }
}

}  // namespace

Client::Client(const Config& config, std::unique_ptr<CredentialsProvider> credentials_provider,
               const std::string& service)
    : config_{config}, credentials_provider_{std::move(credentials_provider)}, signer_{
                                                                                   config.region,
                                                                                   service} {
}

AwsResult<Response> Client::Send(Request req) {
  // TODO(andydunstall): Add retry support.
  return SendAttempt(req);
}

AwsResult<Response> Client::SendAttempt(Request req) {
  req.url.SetScheme((config_.https) ? Scheme::HTTPS : Scheme::HTTP);
  req.headers.emplace("host", req.url.host());

  std::optional<Credentials> creds = credentials_provider_->LoadCredentials();
  if (!creds) {
    return nonstd::make_unexpected(
        AwsError{AwsErrorType::UNAUTHORIZED, "credentials not found", false});
  }

  signer_.SignRequest(*creds, &req);

  AwsResult<Response> resp = client_.Send(req);
  if (!resp) {
    // Network errors.
    return resp;
  }
  if (h2::to_status_class(resp->status) != h2::status_class::successful) {
    if (resp->body.empty()) {
      // If there is no error response body, infer the error from the
      // status code.
      return nonstd::make_unexpected(HttpStatusToAwsError(resp->status));
    }

    // TODO(andydunstall): Handle error body. See S3ErrorMarshaller,
    // XmlErrorMarshaller, and ErrorMarshaller.
    return nonstd::make_unexpected(HttpStatusToAwsError(resp->status));
  }

  return resp;
}

}  // namespace awsv2
}  // namespace util
