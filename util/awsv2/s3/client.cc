// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/awsv2/s3/client.h"

#include <pugixml.hpp>

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

}  // namespace s3
}  // namespace awsv2
}  // namespace util
