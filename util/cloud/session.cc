// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/cloud/session.h"

#include <absl/strings/str_cat.h>
#include <rapidjson/document.h>

#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <cstdlib>
#include <filesystem>

#include "base/logging.h"
#include "io/file.h"
#include "io/line_reader.h"

namespace util {
namespace cloud {

namespace h2 = boost::beast::http;

namespace {

std::string Getenv(const std::string& s) {
  char* env = getenv(s.c_str());
  if (env) {
    return std::string(env);
  }
  return "";
}

std::optional<io::ini::Contents> ReadIniFile(std::string_view path) {
  auto file = io::OpenRead(path, io::ReadonlyFile::Options{});
  if (!file) {
    return std::nullopt;
  }

  io::FileSource file_source{file.value()};
  auto contents = ::io::ini::Parse(&file_source, Ownership::DO_NOT_TAKE_OWNERSHIP);
  if (!contents) {
    LOG(ERROR) << "Load AWS credentials: Failed to parse ini file:" << path;
    return std::nullopt;
  }

  return contents.value();
}

// Make simple GET request on path and return body.
std::optional<std::string> MakeGetRequest(boost::string_view path, http::Client* http_client) {
  h2::request<h2::empty_body> req{h2::verb::get, path, 11};
  h2::response<h2::string_body> resp;
  req.set(h2::field::host, http_client->host());

  std::error_code ec = http_client->Send(req, &resp);
  if (ec || resp.result() != h2::status::ok)
    return std::nullopt;

  VLOG(1) << "Received response: " << resp;
  if (resp[h2::field::connection] == "close") {
    ec = http_client->Reconnect();
    if (ec)
      return std::nullopt;
  }
  return resp.body();
}

}  // namespace

ChainProvider::ChainProvider(const std::vector<std::shared_ptr<Provider>>& providers)
    : providers_{providers} {
}

std::optional<Session> ChainProvider::Load() {
  for (std::shared_ptr<Provider> provider : providers_) {
    std::optional<Session> sess = provider->Load();
    if (sess) {
      return sess;
    }
  }
  return std::nullopt;
}

std::optional<Session> EnvProvider::Load() {
  std::string id = Getenv("AWS_ACCESS_KEY_ID");
  if (id == "") {
    id = Getenv("AWS_ACCESS_KEY");
  }

  std::string secret = Getenv("AWS_SECRET_ACCESS_KEY");
  if (secret == "") {
    secret = Getenv("AWS_SECRET_KEY");
  }

  if (id == "" || secret == "") {
    return std::nullopt;
  }

  Credentials creds;
  creds.access_key_id = id;
  creds.secret_access_key = secret;
  creds.session_token = Getenv("AWS_SESSION_TOKEN");

  Session sess;
  sess.credentials = creds;
  sess.region = Getenv("AWS_REGION");
  if (sess.region.empty()) {
    sess.region = Getenv("AWS_DEFAULT_REGION");
  }

  return sess;
}

SharedConfigProvider::SharedConfigProvider() {
}

std::optional<Session> SharedConfigProvider::Load() {
  const std::string profile = Profile();

  std::optional<Credentials> creds = LoadCredentials(profile);
  if (!creds) {
    return std::nullopt;
  }

  Session sess;
  sess.credentials = *creds;

  std::optional<std::string> region = LoadConfig(profile);
  if (region) {
    sess.region = *region;
  }

  return sess;
}

std::optional<Credentials> SharedConfigProvider::LoadCredentials(const std::string& profile) {
  const std::string filepath = CredentialsFilePath();
  if (filepath.empty()) {
    return std::nullopt;
  }

  std::optional<io::ini::Contents> contents = ReadIniFile(filepath);
  if (!contents) {
    return std::nullopt;
  }

  const auto it = contents->find(profile);
  if (it == contents->end()) {
    LOG(WARNING) << "Load AWS credentials: Failed to find profile in credentials: " << profile;
    return std::nullopt;
  }

  Credentials creds;
  creds.access_key_id = it->second["aws_access_key_id"];
  if (creds.access_key_id == "") {
    return std::nullopt;
  }
  creds.secret_access_key = it->second["aws_secret_access_key"];
  if (creds.secret_access_key == "") {
    return std::nullopt;
  }
  // Default to an empty string if not found.
  creds.session_token = it->second["aws_session_token"];
  return creds;
}

// TODO should this be separate eg can you set region with env var but load
// credentials from config
std::optional<std::string> SharedConfigProvider::LoadConfig(const std::string& profile) {
  const std::string filepath = ConfigFilePath();
  if (filepath.empty()) {
    return std::nullopt;
  }

  std::optional<io::ini::Contents> contents = ReadIniFile(filepath);
  if (contents) {
    const auto it = contents->find(profile);
    if (it != contents->end()) {
      const std::string region = it->second["region"];
      if (!region.empty()) {
        return region;
      }
    }
  }

  // Loading the region from the environment is still allowed even if we read
  // shared credentials.
  // TODO test this matches SDK - couls even add separate config provider?
  std::string region = Getenv("AWS_REGION");
  if (!region.empty()) {
    return region;
  }
  region = Getenv("AWS_DEFAULT_REGION");
  if (!region.empty()) {
    return region;
  }
  return std::nullopt;
}

std::string SharedConfigProvider::CredentialsFilePath() const {
  const std::string filepath = Getenv("AWS_SHARED_CREDENTIALS_FILE");
  if (!filepath.empty()) {
    return filepath;
  }

  const std::string home_dir = Getenv("HOME");
  if (home_dir.empty()) {
    return "";
  }
  return std::filesystem::path{home_dir} / ".aws" / "credentials";
}

std::string SharedConfigProvider::ConfigFilePath() const {
  const std::string filepath = Getenv("AWS_CONFIG_FILE");
  if (!filepath.empty()) {
    return filepath;
  }

  const std::string home_dir = Getenv("HOME");
  if (home_dir.empty()) {
    return "";
  }
  return std::filesystem::path{home_dir} / ".aws" / "config";
}

std::string SharedConfigProvider::Profile() const {
  const std::string profile = Getenv("AWS_PROFILE");
  if (!profile.empty()) {
    return profile;
  }
  return "default";
}

std::optional<Session> Ec2RoleProvider::Load() {
  ProactorBase* pb = ProactorBase::me();
  CHECK(pb);

  http::Client http_client{pb};
  std::error_code ec = http_client.Connect("169.254.169.254", "80");
  if (ec) {
    return std::nullopt;
  }

  std::optional<Credentials> creds = LoadCredentials(&http_client);
  if (!creds) {
    return std::nullopt;
  }

  Session sess;
  sess.credentials = *creds;

  std::optional<std::string> region = LoadConfig(&http_client);
  if (region) {
    sess.region = *region;
  }

  return sess;
}

std::optional<Credentials> Ec2RoleProvider::LoadCredentials(http::Client* http_client) {
  const char* PATH = "/latest/meta-data/iam/security-credentials/";

  if (role_name_.empty()) {
    auto fetched_role = MakeGetRequest(PATH, http_client);
    if (!fetched_role) {
      LOG(ERROR) << "Load AWS credentials: Failed to get role name from metadata";
      return std::nullopt;
    }
    role_name_ = std::move(*fetched_role);
  }

  // Get credentials.
  const std::string path = absl::StrCat(PATH, role_name_);
  auto resp = MakeGetRequest(path, http_client);
  if (!resp) {
    return std::nullopt;
  }

  VLOG(1) << "Load AWS credentials: Received response: " << *resp;

  rapidjson::Document doc;
  doc.Parse(resp->c_str());
  if (!doc.HasMember("AccessKeyId") || !doc.HasMember("SecretAccessKey")) {
    return std::nullopt;
  }

  Credentials creds;
  creds.access_key_id = doc["AccessKeyId"].GetString();
  creds.secret_access_key = doc["SecretAccessKey"].GetString();
  if (doc.HasMember("Token")) {
    creds.session_token = doc["Token"].GetString();
  }

  return creds;
}

std::optional<std::string> Ec2RoleProvider::LoadConfig(http::Client* http_client) {
  const char* PATH = "/latest/dynamic/instance-identity/document";

  auto resp = MakeGetRequest(PATH, http_client);
  if (!resp) {
    return std::nullopt;
  }

  rapidjson::Document doc;
  doc.Parse(resp->c_str());
  if (doc.HasMember("region")) {
    const std::string region = doc["region"].GetString();
    if (!region.empty()) {
      return region;
    }
  }
  return std::nullopt;
}

}  // namespace cloud
}  // namespace util
