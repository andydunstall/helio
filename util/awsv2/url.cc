// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/awsv2/url.h"

#include <absl/strings/ascii.h>
#include <absl/strings/numbers.h>
#include <absl/strings/str_split.h>

#include <iomanip>
#include <sstream>

#include "base/logging.h"

namespace util {
namespace awsv2 {

namespace {

bool IsAlnum(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}

// Encodes the given string given the AWS URL encoding scheme.
std::string UrlEncode(std::string_view s) {
  std::stringstream escaped;
  escaped.fill('0');
  escaped << std::hex << std::uppercase;

  for (char c : s) {
    if (IsAlnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      escaped << (char)c;
    } else {
      escaped << '%' << std::setw(2) << int((unsigned char)c) << std::setw(0);
    }
  }
  return escaped.str();
}

}  // namespace

std::string ToString(Scheme s) {
  if (s == Scheme::HTTP) {
    return "http";
  }
  // Default to HTTPS.
  return "https";
}

Scheme FromString(std::string_view s) {
  const std::string lower_s = absl::AsciiStrToLower(s);
  if (lower_s == "http") {
    return Scheme::HTTP;
  }
  // Default to HTTPS.
  return Scheme::HTTPS;
}

Url::Url() {
}

std::string Url::QueryString() const {
  if (params_.empty()) {
    return "";
  }

  std::string query_string;
  for (const auto& param : params_) {
    query_string += param.first;
    query_string += "=";
    query_string += param.second;
    query_string += "&";
  }
  // Remove the last '&'.
  query_string.pop_back();
  return query_string;
}

void Url::SetScheme(Scheme s) {
  CHECK(s == Scheme::HTTP || s == Scheme::HTTPS);

  scheme_ = s;
  if (port_ == kHttpPort || port_ == kHttpsPort) {
    // If the port is a default HTTP/HTTPS port, update it to match the
    // new scheme. Don't override a custom port.
    if (s == Scheme::HTTP) {
      port_ = kHttpPort;
    }
    if (s == Scheme::HTTPS) {
      port_ = kHttpsPort;
    }
  }
}

void Url::SetHost(std::string_view host) {
  size_t port_idx = host.find(':');
  if (port_idx == std::string_view::npos) {
    host_ = std::string(host);
  } else {
    host_ = std::string(host.substr(0, port_idx));
    uint32_t port;
    CHECK(absl::SimpleAtoi(host.substr(port_idx + 1), &port));
    CHECK_LT(port, 1u << 16);
    port_ = port;
  }
}

void Url::SetPort(uint16_t port) {
  port_ = port;
}

void Url::SetPath(std::string_view path) {
  const std::vector<std::string_view> segments = absl::StrSplit(path, '/');

  std::stringstream ss;
  for (const std::string_view segment : segments) {
    if (segment.empty()) {
      continue;
    }
    ss << '/' << UrlEncode(segment);
  }
  path_ = ss.str();
  if (!path_.empty()) {
    // Remove leading '/'.
    path_ = path_.substr(1);
  }
}

void Url::AddParam(std::string_view k, std::string_view v) {
  params_.emplace(UrlEncode(k), UrlEncode(v));
}

std::string Url::ToString() const {
  std::stringstream ss;
  ss << awsv2::ToString(scheme_);
  ss << "://";
  ss << host_;
  if (port_ != kHttpPort && port_ != kHttpsPort) {
    ss << ":" << port_;
  }
  ss << "/" << path_;
  if (!QueryString().empty()) {
    ss << "?" << QueryString();
  }
  return ss.str();
}

}  // namespace awsv2
}  // namespace util
