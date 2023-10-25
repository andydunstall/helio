// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#pragma once

#include "util/awsv2/credentials.h"
#include "util/http/http_clientv2.h"

namespace util {
namespace awsv2 {

class V4Signer {
 public:
  V4Signer(const std::string& region, const std::string& service, bool sign_payload);

  void SignRequest(const Credentials& credentials, http::Request* req);

 private:
  std::string region_;

  std::string service_;

  bool sign_payload_;
};

}  // namespace awsv2
}  // namespace util
