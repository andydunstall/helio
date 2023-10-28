// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#pragma once

#include <map>
#include <string>

namespace util {
namespace awsv2 {

enum class Scheme {
  HTTP,
  HTTPS,
};

// Url is a AWS compatible URL.
//
// It encodes the URL according to the AWS spec, which is required to ensure
// the V4 signature matches.
class Url {
 public:
  Scheme scheme() const {
    return scheme_;
  }

  std::string host() const {
    return host_;
  }

  std::string path() const {
    return path_;
  }

  uint16_t port() const {
    return port_;
  }

  std::string QueryString() const;

  std::string String() const;

  void SetScheme(Scheme scheme);

  void SetHost(const std::string& host);

  void SetPath(const std::string& path);

  void AddParameter(const std::string& k, const std::string& v);

 private:
  Scheme scheme_;

  std::string host_;

  uint16_t port_ = 0;

  std::string path_ = "/";

  std::map<std::string, std::string> query_;
};

}  // namespace awsv2
}  // namespace util
