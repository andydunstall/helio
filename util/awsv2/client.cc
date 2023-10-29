// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/awsv2/client.h"

namespace util {
namespace awsv2 {

AwsResult<Response> Client::Send(Request* req) {
  Response resp;
  return resp;
}

}  // namespace awsv2
}  // namespace util
