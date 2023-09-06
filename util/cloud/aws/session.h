// Copyright 2022, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#pragma once

#include <string>

#include "util/cloud/aws/signerv4.h"

namespace util {
namespace cloud {
namespace aws {

// Session manages the AWS clients configuration.
class Session {
 public:
  // TODO(andydunstall): Hard coding credentials for now.
  Session(const std::string& access_key_id, const std::string& secret_access_key);

  SignerV4 Signer() const;

 private:
  std::string access_key_id_;
  
  std::string secret_access_key_;
};

}  // namespace aws
}  // namespace cloud
}  // namespace util
