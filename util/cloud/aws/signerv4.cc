// Copyright 2022, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/cloud/aws/signerv4.h"

#include <openssl/evp.h>
#include <openssl/hmac.h>

#include <absl/strings/str_join.h>
#include <glog/logging.h>

#include <iostream>

namespace util {
namespace cloud {
namespace aws {

namespace {

// SHA256 of an empty string.
const char kEmptyStringSHA256[] = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";

void Hexify(const uint8_t* str, size_t len, char* dest) {
  static constexpr char kHex[] = "0123456789abcdef";

  for (unsigned i = 0; i < len; ++i) {
    char c = str[i];
    *dest++ = kHex[(c >> 4) & 0xF];
    *dest++ = kHex[c & 0xF];
  }
  *dest = '\0';
}

void Sha256String(std::string_view str, char out[65]) {
  uint8_t hash[32];
  unsigned temp;

  CHECK_EQ(1, EVP_Digest(str.data(), str.size(), hash, &temp, EVP_sha256(), NULL));

  Hexify(hash, sizeof(hash), out);
}

void HMAC(absl::string_view key, absl::string_view msg, uint8_t dest[32]) {
  // HMAC_xxx are deprecated since openssl 3.0
  // Ubuntu 20.04 uses openssl 1.1.

  unsigned len = 32;
  const uint8_t* data = reinterpret_cast<const uint8_t*>(msg.data());
  ::HMAC(EVP_sha256(), key.data(), key.size(), data, msg.size(), dest, &len);
  CHECK_EQ(len, 32u);
}

struct SigningContext {
  bhttp::header<true, bhttp::fields>* header;
  std::string region;
  std::string service;
  Credentials credentials;
  absl::Time time;
  std::string credential_string;
  std::string body_sha256;
  std::string canonical_headers;
  std::string signed_headers;
  std::string canonical_string;
  std::string string_to_sign;
  std::string signature;

  void BuildTime();
  void BuildCredentialString(const std::string& service, const std::string& region);
  void BuildBodyDigest();
  void BuildCanonicalHeaders();
  void BuildCanonicalString();
  void BuildStringToSign();
  void BuildSignature();
};

std::string FormatAmzTime(absl::Time time) {
  return absl::FormatTime("%Y%m%dT%H%M%SZ", time, absl::UTCTimeZone());
}

std::string FormatAmzShortTime(absl::Time time) {
  return absl::FormatTime("%Y%m%d", time, absl::UTCTimeZone());
}

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

std::string DeriveSigningKey(const std::string& region,
                             const std::string& service,
                             const std::string& secret_key,
                             const std::string& datestamp) {
  std::cout << "DERIVE " << region << " " << service << " " << secret_key << " " << datestamp << std::endl;

  uint8_t sign[32];
  HMAC_CTX* hmac = HMAC_CTX_new();
  unsigned len;

  std::string start_key{"AWS4"};
  std::string_view sign_key{reinterpret_cast<char*>(sign), sizeof(sign)};

  // TODO: to replace with EVP_MAC_CTX_new and EVP_MAC_CTX_free etc which appeared only
  // in openssl 3.0.
  absl::StrAppend(&start_key, secret_key);
  CHECK_EQ(1, HMAC_Init_ex(hmac, start_key.data(), start_key.size(), EVP_sha256(), NULL));
  CHECK_EQ(1,
           HMAC_Update(hmac, reinterpret_cast<const uint8_t*>(datestamp.data()), datestamp.size()));
  CHECK_EQ(1, HMAC_Final(hmac, sign, &len));

  CHECK_EQ(1, HMAC_Init_ex(hmac, sign_key.data(), sign_key.size(), EVP_sha256(), NULL));
  CHECK_EQ(1, HMAC_Update(hmac, reinterpret_cast<const uint8_t*>(region.data()), region.size()));
  CHECK_EQ(1, HMAC_Final(hmac, sign, &len));

  CHECK_EQ(1, HMAC_Init_ex(hmac, sign_key.data(), sign_key.size(), EVP_sha256(), NULL));
  CHECK_EQ(1, HMAC_Update(hmac, reinterpret_cast<const uint8_t*>(service.data()), service.size()));
  CHECK_EQ(1, HMAC_Final(hmac, sign, &len));

  const char* sr = "aws4_request";
  CHECK_EQ(1, HMAC_Init_ex(hmac, sign_key.data(), sign_key.size(), EVP_sha256(), NULL));
  CHECK_EQ(1, HMAC_Update(hmac, reinterpret_cast<const uint8_t*>(sr), strlen(sr)));
  CHECK_EQ(1, HMAC_Final(hmac, sign, &len));

  return std::string(sign_key);
}

void SigningContext::BuildTime() {
  header->set("x-amz-date", FormatAmzTime(time));
}

void SigningContext::BuildCredentialString(const std::string& service,
                                           const std::string& region) {
  credential_string = absl::StrJoin(std::vector<std::string>{
    FormatAmzShortTime(time),
    region,
    service,
    "aws4_request"
  }, "/");
}

void SigningContext::BuildBodyDigest() {
  // If we already have a sha256 header don't recalculate.
  if (header->find("x-amz-content-sha256") != header->end()) {
    body_sha256 = header->find("x-amz-content-sha256")->value();
    return;
  }

  // TODO(andydunstall) Support non-empty bodies.
  body_sha256 = kEmptyStringSHA256;
  header->set("x-amz-content-sha256", body_sha256);
}

void SigningContext::BuildCanonicalHeaders() {
  // TODO: Hard code headers for now.
  canonical_headers = "";

  std::vector<std::string> headers_to_include{
    "host",
    "x-amz-content-sha256",
    "x-amz-date",
  };
  for (const std::string& key : headers_to_include) {
    if (header->find(key) == header->end()) {
      continue;
    }

    std::string value = header->find(key)->value();
    absl::StrAppend(&canonical_headers, absl::StrCat(key, ":", value), "\n");
  }

  signed_headers = absl::StrJoin(headers_to_include, ";");
}

void SigningContext::BuildCanonicalString() {
  // TODO(andydunstall) Support query strings.
  std::string uri = header->target();
  std::string query = "";
  canonical_string = absl::StrJoin(std::vector<std::string>{
    header->method_string(),
    uri,
    query,
    canonical_headers,
    signed_headers,
    body_sha256
  }, "\n");
}

void SigningContext::BuildStringToSign() {
  char canonical_string_sha[65];
  Sha256String(canonical_string, canonical_string_sha);

  string_to_sign = absl::StrJoin(std::vector<std::string>{
    "AWS4-HMAC-SHA256",
    FormatAmzTime(time),
    credential_string,
    canonical_string_sha
  }, "\n");
}

void SigningContext::BuildSignature() {
  std::string signing_key = DeriveSigningKey(
      region,
      service,
      credentials.secret_access_key,
      FormatAmzShortTime(time)
  );

  uint8_t signature_hmac[32];
  char signature_hex[65];
  HMAC(signing_key, string_to_sign, signature_hmac);
  Hexify(signature_hmac, sizeof(signature_hmac), signature_hex);
  signature = std::string(signature_hex);
}

}  // namespace

// TODO(andydunstall) Need to handle refreshing expired credentials.
SignerV4::SignerV4(Credentials credentials) : credentials_{credentials} {}

void SignerV4::Sign(
    bhttp::header<true, bhttp::fields>* header,
    const std::string& service,
    const std::string& region,
    absl::Time sign_time) {
  SigningContext context;
  context.header = header;
  context.credentials = credentials_;
  context.time = sign_time;
  context.region = region;
  context.service = service;

  context.BuildTime();
  context.BuildCredentialString(service, region);
  context.BuildBodyDigest();
  context.BuildCanonicalHeaders();
  context.BuildCanonicalString();
  context.BuildStringToSign();
  context.BuildSignature();

  std::string authorization_header = absl::StrCat(
    "AWS4-HMAC-SHA256", " ",
    "Credential=", credentials_.access_key_id, "/", context.credential_string, ", ",
    "SignedHeaders=", context.signed_headers, ", ",
    "Signature=" + context.signature
  );
  header->set("authorization", authorization_header);

  DVLOG(1) << "canonical string: " << context.canonical_string;
  DVLOG(1) << "string to sign: " << context.string_to_sign;
}

}  // namespace aws
}  // namespace cloud
}  // namespace util
