// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/awsv2/s3/client.h"

#include <pugixml.hpp>

#include "base/logging.h"

namespace util {
namespace awsv2 {
namespace s3 {

namespace h2 = boost::beast::http;

struct ListBucketsResult {
  std::vector<std::string> buckets;

  static AwsResult<ListBucketsResult> Parse(std::string_view s);
};

// TODO unit test...
AwsResult<ListBucketsResult> ListBucketsResult::Parse(std::string_view s) {
  pugi::xml_document doc;
  const pugi::xml_parse_result xml_result = doc.load_buffer(s.data(), s.size());
  if (!xml_result) {
    LOG(ERROR) << "aws: s3 client: failed to parse list buckets response: "
               << xml_result.description();
    // TODO err...
  }

  const pugi::xml_node root = doc.child("ListAllMyBucketsResult");
  if (root.type() != pugi::node_element) {
    LOG(ERROR) << "aws: s3 client: failed to parse list buckets response: count not find root node";
    // TODO err...
  }

  pugi::xml_node buckets = root.child("Buckets");
  if (root.type() != pugi::node_element) {
    LOG(ERROR)
        << "aws: s3 client: failed to parse list buckets response: count not find buckets node";
    // TODO err...
  }

  ListBucketsResult result;
  for (pugi::xml_node node = buckets.child("Bucket"); node; node = node.next_sibling()) {
    result.buckets.push_back(node.child("Name").text().get());
  }
  return result;
}

Client::Client(const std::string& endpoint, bool https, bool ec2_metadata, bool sign_payload)
    : awsv2::Client{endpoint, https, ec2_metadata, sign_payload} {};

AwsResult<std::vector<std::string>> Client::ListBuckets() {
  http::Request req;
  req.method = h2::verb::get;
  req.url.set_path("/");

  AwsResult<http::Response> resp = Send(req);
  if (!resp) {
    return nonstd::make_unexpected(resp.error());
  }

  // TODO handle err
  AwsResult<ListBucketsResult> result = ListBucketsResult::Parse(resp->body);
  if (!result) {
    // TODO ...
  }
  return result->buckets;
}

AwsResult<std::vector<std::string>> Client::ListObjects(std::string_view bucket,
                                                        std::string_view prefix) {
  return std::vector<std::string>{};
}

}  // namespace s3
}  // namespace awsv2
}  // namespace util
