// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/awsv2/v4_signer.h"

#include <absl/strings/escaping.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include <sstream>

#include "base/logging.h"

namespace util {
namespace awsv2 {

namespace h2 = boost::beast::http;

namespace {

const std::string kUnsignedPayload = "UNSIGNED-PAYLOAD";

std::string CanonicalHeaders(const std::map<std::string, std::string>& headers) {
  std::stringstream ss;
  // Note headers are kept in sorted order.
  for (const auto& header : headers) {
    ss << header.first.c_str() << ":" << header.second.c_str() << "\n";
  }
  return ss.str();
}

std::string SignedHeaders(const std::map<std::string, std::string>& headers) {
  std::stringstream ss;
  // Note headers are kept in sorted order.
  for (const auto& header : headers) {
    ss << header.first.c_str() << ";";
  }
  std::string s = ss.str();
  // Remove the last semi-colon.
  if (!s.empty()) {
    s.pop_back();
  }
  return s;
}

std::string CanonicalRequest(const std::string& canonical_headers,
                             const std::string& signed_headers, const std::string& payload_hash,
                             Request* req) {
  // TODO(andydunstall): URL encode path.
  const std::string path = (req->url.path().empty()) ? "/" : req->url.path();

  std::stringstream ss;
  ss << h2::to_string(req->method) << "\n";
  ss << path << "\n";
  // TODO(andydunstall): Support query string.
  ss << "\n";
  ss << canonical_headers << "\n";
  ss << signed_headers << "\n";
  ss << payload_hash;

  return ss.str();
}

constexpr size_t kSha256Size = 256 / 8;

std::string Sha256String(std::string_view s) {
  uint8_t sha256[kSha256Size];
  unsigned size;
  CHECK_EQ(1, EVP_Digest(s.data(), s.size(), sha256, &size, EVP_sha256(), nullptr));
  return absl::BytesToHexString(
      std::string_view(reinterpret_cast<const char*>(sha256), kSha256Size));
}

std::array<uint8_t, kSha256Size> HmacSha256(std::string_view data, std::string_view key) {
  std::array<uint8_t, kSha256Size> hmac;
  unsigned hmac_len;
  CHECK_NE(nullptr, HMAC(EVP_sha256(), key.data(), key.size(),
                         reinterpret_cast<const unsigned char*>(data.data()), data.size(),
                         hmac.data(), &hmac_len));
  return hmac;
}

}  // namespace

V4Signer::V4Signer(const std::string& region, const std::string& service)
    : region_{region}, service_{service} {
}

void V4Signer::SignRequest(const Credentials& credentials, Request* req,
                           const absl::Time& time) const {
  if (!credentials.session_token.empty()) {
    req->headers.emplace("x-amz-security-token", credentials.session_token);
  }

  std::string payload_hash = kUnsignedPayload;
  // TODO(andydunstall): Support payload signature over HTTP.
  // if (sign_payload_ && req->url.scheme() != "https") {
  //   ...
  // }

  const std::string date_header = absl::FormatTime("%Y%m%dT%H%M00Z", time, absl::UTCTimeZone());
  req->headers.emplace("x-amz-date", date_header);
  req->headers.emplace("x-amz-content-sha256", payload_hash);

  const std::string canonical_headers = CanonicalHeaders(req->headers);
  VLOG(1) << "aws: v4 signer: canonical headers: " << canonical_headers;

  const std::string signed_headers = SignedHeaders(req->headers);
  VLOG(1) << "aws: v4 signer: signed headers: " << signed_headers;

  const std::string canonical_request =
      CanonicalRequest(canonical_headers, signed_headers, payload_hash, req);
  VLOG(1) << "aws: v4 signer: canonical request: " << canonical_request;

  const std::string string_to_sign = StringToSign(canonical_request, time);
  VLOG(1) << "aws: v4 signer: string to sign: " << string_to_sign;

  const std::string signature = Signature(credentials, string_to_sign, time);
  VLOG(1) << "aws: v4 signer: signature: " << signature;

  const std::string auth_string = AuthHeader(credentials, signed_headers, signature, time);
  VLOG(1) << "aws: v4 signer: auth string: " << auth_string;

  req->headers.emplace("authorization", auth_string);
}

std::string V4Signer::StringToSign(const std::string& canonical_request,
                                   const absl::Time& time) const {
  const std::string date_header = absl::FormatTime("%Y%m%dT%H%M00Z", time, absl::UTCTimeZone());

  const std::string canonical_request_hash = Sha256String(canonical_request);

  const std::string simple_date = absl::FormatTime("%Y%m%d", time, absl::UTCTimeZone());

  std::stringstream ss;
  ss << "AWS4-HMAC-SHA256"
     << "\n";
  ss << date_header << "\n";
  ss << simple_date << "/" << region_ << "/" << service_ << "/"
     << "aws4_request"
     << "\n";
  ss << canonical_request_hash;
  return ss.str();
}

std::string V4Signer::Signature(const Credentials& credentials, const std::string& string_to_sign,
                                const absl::Time& time) const {
  const std::string simple_date = absl::FormatTime("%Y%m%d", time, absl::UTCTimeZone());

  std::string signing_key = "AWS4";
  signing_key.append(credentials.secret_access_key);
  std::array<uint8_t, kSha256Size> hmac;
  hmac = HmacSha256(simple_date, signing_key);
  hmac = HmacSha256(region_, std::string_view((const char*)hmac.data(), hmac.size()));
  hmac = HmacSha256(service_, std::string_view((const char*)hmac.data(), hmac.size()));
  hmac = HmacSha256("aws4_request", std::string_view((const char*)hmac.data(), hmac.size()));
  hmac = HmacSha256(string_to_sign, std::string_view((const char*)hmac.data(), hmac.size()));

  return absl::BytesToHexString(
      std::string_view(reinterpret_cast<const char*>(hmac.data()), hmac.size()));
}

std::string V4Signer::AuthHeader(const Credentials& credentials, const std::string& signed_headers,
                                 const std::string signature, const absl::Time& time) const {
  const std::string simple_date = absl::FormatTime("%Y%m%d", time, absl::UTCTimeZone());

  std::stringstream ss;
  ss << "AWS4-HMAC-SHA256"
     << " "
     << "Credential=" << credentials.access_key_id << "/" << simple_date << "/" << region_ << "/"
     << service_ << "/"
     << "aws4_request"
     << ", "
     << "SignedHeaders=" << signed_headers << ", "
     << "Signature=" << signature;
  return ss.str();
}

}  // namespace awsv2
}  // namespace util
