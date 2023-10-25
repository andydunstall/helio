// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/http/http_clientv2.h"

#include "base/logging.h"

namespace util {
namespace http {

HttpResult<Response> ClientV2::Send(const Request& req) {
  VLOG(1) << "http client: send request; method=" << h2::to_string(req.method)
          << "; url=" << req.url.buffer();

  return {};
}

}  // namespace http
}  // namespace util
