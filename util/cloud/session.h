// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "util/http/http_client.h"

namespace util {
namespace cloud {

struct Credentials {
  std::string access_key_id;

  std::string secret_access_key;

  std::string session_token;
};

struct Session {
  Credentials credentials;

  std::string region;
};

class Provider {
 public:
  virtual ~Provider() = default;

  virtual std::optional<Session> Load() = 0;
};

class ChainProvider : public Provider {
 public:
  ChainProvider(const std::vector<std::shared_ptr<Provider>>& providers = {});

  std::optional<Session> Load() override;

 private:
  std::vector<std::shared_ptr<Provider>> providers_;
};

class EnvProvider : public Provider {
 public:
  std::optional<Session> Load() override;
};

// TODO rename shared config
class SharedConfigProvider : public Provider {
 public:
  SharedConfigProvider();

  std::optional<Session> Load() override;

 private:
  std::optional<Credentials> LoadCredentials(const std::string& profile);

  std::optional<std::string> LoadConfig(const std::string& profile);

  // Returns the path to the users credentials file or an empty string if the
  // path is unknown.
  std::string CredentialsFilePath() const;

  // Returns the path to the users config file or an empty string if the
  // path is unknown.
  std::string ConfigFilePath() const;

  // Returns the shared credentials profile.
  std::string Profile() const;
};

class Ec2RoleProvider : public Provider {
 public:
  // Load must be run from a proactor thread.
  std::optional<Session> Load() override;

 private:
  // TODO will this handle retries etc
  std::optional<Credentials> LoadCredentials(http::Client* http_client);

  std::optional<std::string> LoadConfig(http::Client* http_client);

  std::string role_name_;
};

}  // namespace cloud
}  // namespace util
