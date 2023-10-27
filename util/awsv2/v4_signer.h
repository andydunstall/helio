// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#pragma once

#include <absl/time/clock.h>

#include "util/awsv2/credentials.h"
#include "util/http/clientv2.h"

namespace util {
namespace awsv2 {

// V4 signer used to authenticate AWS requests.
//
// See https://docs.aws.amazon.com/AmazonS3/latest/API/sig-v4-authenticating-requests.html
// for details.
class V4Signer {
 public:
  V4Signer(const std::string& region, const std::string& service);

  // Signs the request with the given credentials.
  void SignRequest(const Credentials& credentials, http::Request* req,
                   const absl::Time& time = absl::Now()) const;

 private:
  std::string StringToSign(const std::string& canonical_request, const absl::Time& time) const;

  std::string Signature(const Credentials& credentials, const std::string& string_to_sign,
                        const absl::Time& time) const;

  std::string AuthHeader(const Credentials& credentials, const std::string& signed_headers,
                         const std::string signature, const absl::Time& time) const;

  std::string region_;

  std::string service_;
};

}  // namespace awsv2
}  // namespace util
