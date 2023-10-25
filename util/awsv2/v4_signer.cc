// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/awsv2/v4_signer.h"

#include <absl/strings/escaping.h>
#include <absl/time/clock.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include <boost/beast/http/verb.hpp>

#include "base/logging.h"

namespace util {
namespace awsv2 {

namespace h2 = boost::beast::http;

namespace {

const std::string kUnsignedPayload = "UNSIGNED-PAYLOAD";

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

V4Signer::V4Signer(const std::string& region, const std::string& service, bool sign_payload)
    : region_{region}, service_{service}, sign_payload_{sign_payload} {
}

void V4Signer::SignRequest(const Credentials& credentials, http::Request* req) {
  if (!credentials.session_token.empty()) {
    req->headers.emplace("x-amz-security-token", credentials.session_token);
  }

  std::string payload_hash = kUnsignedPayload;
  // If configured and the request is not sent over HTTPS sign the request
  // payload.
  if (sign_payload_ && req->url.scheme() != "https") {
    // TODO(andydunstall): Sign payload.
  }

  absl::Time now = absl::Now();
  std::string date_header = absl::FormatTime("%Y%m%dT%H%M00Z", now, absl::UTCTimeZone());
  req->headers.emplace("x-amz-date", date_header);
  req->headers.emplace("x-amz-content-sha256", payload_hash);

  std::stringstream headers_stream;
  std::stringstream signed_headers_stream;
  for (const auto& header : req->headers) {
    headers_stream << header.first.c_str() << ":" << header.second.c_str() << "\n";
    signed_headers_stream << header.first.c_str() << ";";
  }

  const std::string canonical_headers = headers_stream.str();
  VLOG(1) << "aws: v4 signer: canonical headers: " << canonical_headers;

  std::string signed_headers = signed_headers_stream.str();
  // Remove the last semi-colon.
  if (!signed_headers.empty()) {
    signed_headers.pop_back();
  }
  VLOG(1) << "aws: v4 signer: signed headers: " << signed_headers;

  std::stringstream canonical_request_stream;
  canonical_request_stream << h2::to_string(req->method) << "\n";
  // TODO(andydunstall): URL encode path.
  canonical_request_stream << req->url.path() << "\n";
  // TODO(andydunstall): Support query string.
  canonical_request_stream << "\n";
  canonical_request_stream << canonical_headers << "\n";
  canonical_request_stream << signed_headers << "\n";
  canonical_request_stream << payload_hash << "\n";

  const std::string canonical_request = canonical_request_stream.str();
  VLOG(1) << "aws: v4 signer: canonical request: " << canonical_request;

  std::string canonical_request_hash = Sha256String(canonical_request);
  std::string simple_date = absl::FormatTime("%Y%m%d", now, absl::UTCTimeZone());

  std::stringstream string_to_sign_stream;
  string_to_sign_stream << "AWS4-HMAC-SHA256"
                        << "\n";
  string_to_sign_stream << simple_date << "\n";
  // TODO(andydunstall): Configure region and service.
  string_to_sign_stream << region_ << "/" << service_ << "/"
                        << "aws4_request"
                           "\n";
  string_to_sign_stream << canonical_request_hash;
  const std::string string_to_sign = string_to_sign_stream.str();

  VLOG(1) << "aws: v4 signer: string to sign: " << string_to_sign;

  std::string signing_key = "AWS4";
  signing_key.append(credentials.secret_access_key);
  std::array<uint8_t, kSha256Size> hmac;
  hmac = HmacSha256(simple_date, signing_key);
  hmac = HmacSha256(region_, std::string_view((const char*)hmac.data(), hmac.size()));
  hmac = HmacSha256(service_, std::string_view((const char*)hmac.data(), hmac.size()));
  hmac = HmacSha256("aws4_request", std::string_view((const char*)hmac.data(), hmac.size()));

  hmac = HmacSha256(string_to_sign, std::string_view((const char*)hmac.data(), hmac.size()));

  const std::string signature = absl::BytesToHexString(
      std::string_view(reinterpret_cast<const char*>(hmac.data()), hmac.size()));
  VLOG(1) << "aws: v4 signer: signature: " << signature;

  std::stringstream auth_string_stream;
  auth_string_stream << "AWS4-HMAC-SHA256"
                     << " "
                     << "Credential=" << credentials.access_key_id << "/" << simple_date << "/"
                     << region_ << "/" << service_ << "/"
                     << "aws4_request"
                     << ", "
                     << "SignedHeaders=" << signed_headers << ", "
                     << "Signature=" << signature;
  const std::string auth_string = auth_string_stream.str();
  VLOG(1) << "aws: v4 signer: auth string: " << auth_string;

  req->headers.emplace("authorization", auth_string);
}

}  // namespace awsv2
}  // namespace util
