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
    LOG(ERROR) << "aws: s3 client: failed to parse list buckets response: "
               << xml_result.description();
    return nonstd::make_unexpected(AwsError::INVALID_RESPONSE);
  }

  const pugi::xml_node root = doc.child("ListAllMyBucketsResult");
  if (root.type() != pugi::node_element) {
    LOG(ERROR) << "aws: s3 client: failed to parse list buckets response: count not find root node";
    return nonstd::make_unexpected(AwsError::INVALID_RESPONSE);
  }

  pugi::xml_node buckets = root.child("Buckets");
  if (root.type() != pugi::node_element) {
    LOG(ERROR)
        << "aws: s3 client: failed to parse list buckets response: count not find buckets node";
    return nonstd::make_unexpected(AwsError::INVALID_RESPONSE);
  }

  ListBucketsResult result;
  for (pugi::xml_node node = buckets.child("Bucket"); node; node = node.next_sibling()) {
    result.buckets.push_back(node.child("Name").text().get());
  }
  return result;
}

Client::Client(const std::string& region) : awsv2::Client{region, "s3"} {
}

AwsResult<std::vector<std::string>> Client::ListBuckets() {
  Request req;
  req.method = h2::verb::get;
  req.url.set_host("s3.amazonaws.com");
  req.headers.emplace("host", "s3.amazonaws.com");

  AwsResult<std::string> resp = Send(&req);
  if (!resp) {
    return nonstd::make_unexpected(resp.error());
  }

  AwsResult<ListBucketsResult> result = ListBucketsResult::Parse(*resp);
  if (!result) {
    return nonstd::make_unexpected(result.error());
  }

  return result->buckets;
}

}  // namespace s3
}  // namespace awsv2
}  // namespace util
