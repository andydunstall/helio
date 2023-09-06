// Copyright 2022, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#pragma once

#include <absl/time/clock.h>
#include <absl/time/time.h>
#include <boost/beast/http/message.hpp>

#include "util/cloud/aws/credentials.h"

namespace util {
namespace cloud {
namespace aws {

namespace bhttp = boost::beast::http;

// SignerV4 provides request signing with AWS V4 signatures.
//
// See https://docs.aws.amazon.com/AmazonS3/latest/API/sig-v4-authenticating-requests.html
// for details.
class SignerV4 {
 public:
  SignerV4(Credentials credentials);

  void Sign(bhttp::header<true, bhttp::fields>* header,
            const std::string& service,
            const std::string& region,
            absl::Time sign_time = absl::Now());

 private:
  Credentials credentials_;
};

}  // namespace aws
}  // namespace cloud
}  // namespace util
