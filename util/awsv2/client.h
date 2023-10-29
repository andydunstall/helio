// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#pragma once

#include "util/awsv2/aws.h"

namespace util {
namespace awsv2 {

class Client {
 public:
  AwsResult<Response> Send(Request* req);
};

}  // namespace awsv2
}  // namespace util
