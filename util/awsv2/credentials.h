// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#pragma once

#include <chrono>
#include <string>

namespace util {
namespace awsv2 {

struct Credentials {
  std::string access_key_id;
  std::string secret_access_key;
  std::string session_token;
};

}  // namespace awsv2
}  // namespace util
