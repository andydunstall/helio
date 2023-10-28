// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/awsv2/url.h"

#include <absl/strings/str_split.h>

#include <cstring>
#include <iomanip>
#include <sstream>

namespace util {
namespace awsv2 {

namespace {

bool IsAlnum(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}

std::string UrlEncode(const char* unsafe) {
  std::stringstream escaped;
  escaped.fill('0');
  escaped << std::hex << std::uppercase;

  size_t unsafeLength = strlen(unsafe);
  for (auto i = unsafe, n = unsafe + unsafeLength; i != n; ++i) {
    char c = *i;
    if (IsAlnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      escaped << (char)c;
    } else {
      // this unsigned char cast allows us to handle unicode characters.
      escaped << '%' << std::setw(2) << int((unsigned char)c) << std::setw(0);
    }
  }

  return escaped.str();
}

}  // namespace

std::string Url::QueryString() const {
  if (query_.empty()) {
    return "";
  }

  std::string query_string;
  for (const auto& param : query_) {
    query_string += param.first;
    query_string += "=";
    query_string += param.second;
    query_string += "&";
  }
  // Remove the last '&'.
  query_string.pop_back();
  return query_string;
}

std::string Url::String() const {
  std::stringstream ss;
  if (scheme_ == Scheme::HTTP) {
    ss << "http";
  } else if (scheme_ == Scheme::HTTPS) {
    ss << "https";
  }
  ss << "://";
  ss << host_;
  ss << path_;
  if (!QueryString().empty()) {
    ss << "?";
    ss << QueryString();
  }
  return ss.str();
}

void Url::SetScheme(Scheme scheme) {
  scheme_ = scheme;
  if (port_ == 0) {
    if (scheme_ == Scheme::HTTP) {
      port_ = 80;
    } else {
      port_ = 443;
    }
  }
}

void Url::SetHost(const std::string& host) {
  host_ = host;
}

void Url::SetPath(const std::string& path) {
  std::stringstream ss;
  const std::vector<std::string> segments = absl::StrSplit(path, "/");
  for (const std::string& segment : segments) {
    ss << UrlEncode(segment.c_str()) << "/";
  }
  std::string path_encoded = ss.str();
  path_encoded.pop_back();
  path_ = path_encoded;
}

void Url::AddParameter(const std::string& k, const std::string& v) {
  query_.emplace(UrlEncode(k.c_str()), UrlEncode(v.c_str()));
}

}  // namespace awsv2
}  // namespace util
