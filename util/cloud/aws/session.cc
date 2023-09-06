// Copyright 2022, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/cloud/aws/session.h"

namespace util {
namespace cloud {
namespace aws {

Session::Session(const std::string& access_key_id, const std::string& secret_access_key)
  : access_key_id_{access_key_id},
    secret_access_key_{secret_access_key} {}

SignerV4 Session::Signer() const {
  Credentials creds;
  creds.access_key_id = access_key_id_;
  creds.secret_access_key = secret_access_key_;
  return SignerV4{creds};
}

}  // namespace aws
}  // namespace cloud
}  // namespace util
