// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/awsv2/s3/client.h"

namespace util {
namespace awsv2 {
namespace s3 {

namespace h2 = boost::beast::http;

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

  // TODO(andydunstall): ListBucketsResponse::Parse(resp.body)

  return std::vector<std::string>{};
}

AwsResult<std::vector<std::string>> Client::ListObjects(std::string_view bucket,
                                                        std::string_view prefix) {
  return std::vector<std::string>{};
}

}  // namespace s3
}  // namespace awsv2
}  // namespace util
