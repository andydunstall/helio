// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/awsv2/s3/client.h"

#include <pugixml.hpp>

#include "base/logging.h"

namespace util {
namespace awsv2 {
namespace s3 {

AwsResult<ListBucketsResult> ListBucketsResult::Parse(std::string_view s) {
  pugi::xml_document doc;
  const pugi::xml_parse_result xml_result = doc.load_buffer(s.data(), s.size());
  if (!xml_result) {
    return nonstd::make_unexpected(AwsError{AwsErrorType::INVALID_RESPONSE,
                                            "parse list buckets response: invalid xml", false});
  }

  const pugi::xml_node root = doc.child("ListAllMyBucketsResult");
  if (root.type() != pugi::node_element) {
    return nonstd::make_unexpected(
        AwsError{AwsErrorType::INVALID_RESPONSE,
                 "parse list buckets response: ListAllMyBucketsResult not found", false});
  }

  pugi::xml_node buckets = root.child("Buckets");
  if (root.type() != pugi::node_element) {
    return nonstd::make_unexpected(AwsError{
        AwsErrorType::INVALID_RESPONSE, "parse list buckets response: Buckets not found", false});
  }

  ListBucketsResult result;
  for (pugi::xml_node node = buckets.child("Bucket"); node; node = node.next_sibling()) {
    result.buckets.push_back(node.child("Name").text().get());
  }
  return result;
}

AwsResult<ListObjectsResult> ListObjectsResult::Parse(std::string_view s) {
  pugi::xml_document doc;
  const pugi::xml_parse_result xml_result = doc.load_buffer(s.data(), s.size());
  if (!xml_result) {
    return nonstd::make_unexpected(AwsError{AwsErrorType::INVALID_RESPONSE,
                                            "parse list objects response: invalid xml", false});
  }

  const pugi::xml_node root = doc.child("ListBucketResult");
  if (root.type() != pugi::node_element) {
    return nonstd::make_unexpected(
        AwsError{AwsErrorType::INVALID_RESPONSE,
                 "parse list objects response: ListBucketsResult not found", false});
  }

  ListObjectsResult result;
  for (pugi::xml_node node = root.child("Contents"); node; node = node.next_sibling("Contents")) {
    result.objects.push_back(node.child("Key").text().get());
  }

  bool truncated = root.child("IsTruncated").text().as_bool();
  if (truncated) {
    result.continuation_token = root.child("NextContinuationToken").text().get();
  }

  return result;
}

Client::Client(const Config& config,
               std::unique_ptr<util::awsv2::CredentialsProvider> credentials_provider)
    : awsv2::Client{config, std::move(credentials_provider), "s3"} {
}

AwsResult<std::vector<std::string>> Client::ListBuckets() {
  Request req;
  req.method = h2::verb::get;
  req.url.SetHost("s3.amazonaws.com");

  AwsResult<Response> resp = Send(req);
  if (!resp) {
    return nonstd::make_unexpected(resp.error());
  }

  AwsResult<ListBucketsResult> result = ListBucketsResult::Parse(resp->body);
  if (!result) {
    return nonstd::make_unexpected(result.error());
  }

  return result->buckets;
}

AwsResult<std::vector<std::string>> Client::ListObjects(std::string_view bucket,
                                                        std::string_view prefix, size_t limit) {
  std::vector<std::string> objects;
  std::string continuation_token;
  do {
    Request req;
    req.method = h2::verb::get;
    req.url.SetHost(std::string(bucket) + ".s3.amazonaws.com");
    req.url.SetPath("/");
    req.headers.emplace("host", std::string(bucket) + ".s3.amazonaws.com");

    // ListObjectsV2.
    req.url.AddParam("list-type", "2");

    if (!prefix.empty()) {
      req.url.AddParam("prefix", std::string(prefix));
    }

    if (limit > 0) {
      req.url.AddParam("max-keys", absl::StrFormat("%d", limit - objects.size()));
    }
    if (!continuation_token.empty()) {
      req.url.AddParam("continuation-token", continuation_token);
    }

    AwsResult<Response> resp = Send(req);
    if (!resp) {
      return nonstd::make_unexpected(resp.error());
    }

    AwsResult<ListObjectsResult> result = ListObjectsResult::Parse(resp->body);
    if (!result) {
      return nonstd::make_unexpected(result.error());
    }

    VLOG(1) << "aws: list objects; objects=" << result->objects.size()
            << "; continuation_token=" << result->continuation_token;

    objects.insert(objects.end(), result->objects.begin(), result->objects.end());
    continuation_token = result->continuation_token;

    if (limit > 0 && objects.size() >= limit) {
      return objects;
    }
  } while (!continuation_token.empty());
  return objects;
}

}  // namespace s3
}  // namespace awsv2
}  // namespace util
