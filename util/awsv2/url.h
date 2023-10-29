// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#pragma once

#include <map>
#include <string>

namespace util {
namespace awsv2 {

constexpr uint16_t kHttpPort = 80;
constexpr uint16_t kHttpsPort = 443;

enum class Scheme { HTTP, HTTPS };

std::string ToString(Scheme s);

Scheme FromString(std::string_view s);

// Url is a URL for AWS requests.
//
// It encodes the URL to be compatible with AWS V4 signatures.
class Url {
 public:
  Url();

  Scheme scheme() const {
    return scheme_;
  }

  std::string host() const {
    return host_;
  }

  uint16_t port() const {
    return port_;
  }

  // Returns the URL encoded path.
  std::string path() const {
    return path_;
  }

  std::string QueryString() const;

  void SetScheme(Scheme s);

  void SetHost(std::string_view host);

  void SetPort(uint16_t port);

  void SetPath(std::string_view path);

  void AddParam(std::string_view k, std::string_view v);

  std::string ToString() const;

 private:
  Scheme scheme_ = Scheme::HTTPS;

  std::string host_;

  uint16_t port_ = kHttpsPort;

  // URL encoded path.
  std::string path_;

  // Sorted query string parameters.
  std::map<std::string, std::string> params_;
};

}  // namespace awsv2
}  // namespace util
