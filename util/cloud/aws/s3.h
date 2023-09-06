// Copyright 2022, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#pragma once

#include <string>
#include <vector>

#include "io/io.h"
#include "util/cloud/aws/session.h"
#include "util/http/http_client.h"

namespace util {
namespace cloud {
namespace aws {

class S3 {
 public:
  S3(Session* session, util::http::Client* http_client);

  io::Result<std::vector<std::string>> ListBuckets();

 private:
  Session* session_;

  util::http::Client* http_client_;
};

}  // namespace aws
}  // namespace cloud
}  // namespace util
