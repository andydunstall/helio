// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/cloud/client.h"

#include <chrono>

using namespace std::chrono_literals;

namespace util {
namespace cloud {

Client::Client(const AWS* aws, http::Client* http_client) : aws_{aws}, client_{http_client} {
  sign_key_ = aws_->GetSignKey(aws_->connection_data().region);
}

void Client::RetryBackoff(int attempt) const {
  if (attempt <= 1) {
    return;
  }

  // TODO(andydunstall) Hard code for now
  ThisFiber::SleepFor(5s);
}

}  // namespace cloud
}  // namespace util
