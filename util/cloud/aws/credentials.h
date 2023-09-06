// Copyright 2022, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#pragma once

#include <string>

namespace util {
namespace cloud {
namespace aws {

struct Credentials {
	// AWS Access key ID
  std::string access_key_id;

	// AWS Secret Access Key
  std::string secret_access_key;

	// AWS Session Token
  std::string session_token;
};

}  // namespace aws
}  // namespace cloud
}  // namespace util
