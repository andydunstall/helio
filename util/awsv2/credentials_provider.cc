// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/awsv2/credentials_provider.h"

#include <cstdlib>

#include "base/logging.h"

namespace util {
namespace awsv2 {

namespace {

std::string GetEnv(const char* name) {
  char* value = std::getenv(name);
  if (value) {
    return std::string(value);
  }
  return "";
}

}  // namespace

std::optional<Credentials> EnvironmentCredentialsProvider::LoadCredentials() {
  Credentials creds;

  creds.access_key_id = GetEnv("AWS_ACCESS_KEY_ID");
  if (creds.access_key_id.empty()) {
    VLOG(1) << "aws: environment credentials provider: missing access key id";
    return std::nullopt;
  }

  creds.secret_access_key = GetEnv("AWS_SECRET_ACCESS_KEY");
  if (creds.secret_access_key.empty()) {
    VLOG(1) << "aws: environment credentials provider: secret access key";
    return std::nullopt;
  }

  creds.session_token = GetEnv("AWS_SESSION_TOKEN");

  LOG(INFO) << "aws: environment credentials provider: loaded credentials";

  return creds;
}

}  // namespace awsv2
}  // namespace util
