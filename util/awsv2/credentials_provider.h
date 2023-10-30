// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "util/awsv2/credentials.h"

namespace util {
namespace awsv2 {

class CredentialsProvider {
 public:
  CredentialsProvider(const std::string& name);

  virtual ~CredentialsProvider() = default;

  std::string name() const {
    return name_;
  }

  // Looks up the credentials for this provider. Returns valid credentials fi
  // found, otherwise returns nullopt.
  virtual std::optional<Credentials> LoadCredentials() = 0;

 private:
  std::string name_;
};

// Reads AWS credentials from environment variable AWS_ACCESS_KEY_ID,
// AWS_SECRET_ACCESS_KEY and AWS_SESSION_TOKEN if they exist.
class EnvironmentCredentialsProvider : public CredentialsProvider {
 public:
  EnvironmentCredentialsProvider();

  std::optional<Credentials> LoadCredentials() override;
};

// Loads a credentials chain of providers.
//
// Attempts to load credentials from each provider in order, and returns the
// first credentials found.
class CredentialsProviderChain : public CredentialsProvider {
 public:
  CredentialsProviderChain();

  std::optional<Credentials> LoadCredentials() override;

  void AddProvider(std::unique_ptr<CredentialsProvider> provider);

  static std::unique_ptr<CredentialsProvider> DefaultCredentialProviderChain();

 private:
  std::vector<std::unique_ptr<CredentialsProvider>> providers_;
};

}  // namespace awsv2
}  // namespace util
