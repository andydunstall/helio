// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/awsv2/credentials_provider.h"

#include "base/gtest.h"

namespace util {
namespace awsv2 {

class EnvironmentCredentialsProviderTest : public ::testing::Test {};

TEST_F(EnvironmentCredentialsProviderTest, Found) {
  EnvironmentCredentialsProvider provider;

  setenv("AWS_ACCESS_KEY_ID", "key", 1);
  setenv("AWS_SECRET_ACCESS_KEY", "secret", 1);
  setenv("AWS_SESSION_TOKEN", "token", 1);

  const std::optional<Credentials> creds = provider.GetCredentials();
  EXPECT_NE(std::nullopt, creds);
  EXPECT_EQ("key", creds->access_key_id);
  EXPECT_EQ("secret", creds->secret_access_key);
  EXPECT_EQ("token", creds->session_token);
}

TEST_F(EnvironmentCredentialsProviderTest, NotFound) {
  EnvironmentCredentialsProvider provider;

  unsetenv("AWS_ACCESS_KEY_ID");
  unsetenv("AWS_SECRET_ACCESS_KEY");
  EXPECT_EQ(std::nullopt, provider.GetCredentials());
}

}  // namespace awsv2
}  // namespace util
