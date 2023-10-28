// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/awsv2/s3/client.h"

#include <absl/strings/str_format.h>
#include <absl/strings/str_join.h>
#include <absl/strings/str_split.h>

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
    LOG(ERROR)
        << "aws: s3 client: failed to parse list buckets response: buckets not find root node";
    return nonstd::make_unexpected(AwsError::INVALID_RESPONSE);
  }

  pugi::xml_node buckets = root.child("Buckets");
  if (root.type() != pugi::node_element) {
    LOG(ERROR)
        << "aws: s3 client: failed to parse list buckets response: buckets not find buckets node";
    return nonstd::make_unexpected(AwsError::INVALID_RESPONSE);
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
    LOG(ERROR) << "aws: s3 client: failed to parse list objects response: "
               << xml_result.description();
    return nonstd::make_unexpected(AwsError::INVALID_RESPONSE);
  }

  const pugi::xml_node root = doc.child("ListBucketResult");
  if (root.type() != pugi::node_element) {
    LOG(ERROR)
        << "aws: s3 client: failed to parse list objects response: objects not find root node";
    return nonstd::make_unexpected(AwsError::INVALID_RESPONSE);
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

// TODO(andydunstall): Unit test.
AwsResult<CreateMultipartUploadResult> CreateMultipartUploadResult::Parse(std::string_view s) {
  pugi::xml_document doc;
  const pugi::xml_parse_result xml_result = doc.load_buffer(s.data(), s.size());
  if (!xml_result) {
    LOG(ERROR) << "aws: s3 client: failed to parse create multipart upload response: "
               << xml_result.description();
    return nonstd::make_unexpected(AwsError::INVALID_RESPONSE);
  }

  const pugi::xml_node root = doc.child("InitiateMultipartUploadResult");
  if (root.type() != pugi::node_element) {
    LOG(ERROR) << "aws: s3 client: failed to parse list objects response: root node not found";
    return nonstd::make_unexpected(AwsError::INVALID_RESPONSE);
  }

  CreateMultipartUploadResult result;
  result.upload_id = root.child("UploadId").text().get();
  return result;
}

Client::Client(const std::string& region) : awsv2::Client{region, "s3"} {
}

AwsResult<std::vector<std::string>> Client::ListBuckets() {
  Request req;
  req.method = h2::verb::get;
  req.url2.SetHost("s3.amazonaws.com");
  req.url2.SetPath("/");
  req.headers.emplace("host", "s3.amazonaws.com");

  AwsResult<Response> resp = Send(&req);
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
    req.url2.SetHost(std::string(bucket) + ".s3.amazonaws.com");
    req.url2.SetPath("/");
    req.headers.emplace("host", std::string(bucket) + ".s3.amazonaws.com");

    // ListObjectsV2.
    req.url2.AddParameter("list-type", "2");

    if (!prefix.empty()) {
      req.url2.AddParameter("prefix", std::string(prefix));
    }

    if (limit > 0) {
      req.url2.AddParameter("max-keys", absl::StrFormat("%d", limit - objects.size()));
    }
    if (!continuation_token.empty()) {
      req.url2.AddParameter("continuation-token", continuation_token);
    }

    AwsResult<Response> resp = Send(&req);
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

AwsResult<GetObjectResult> Client::GetObject(std::string_view bucket, std::string_view key,
                                             std::string_view range) {
  Request req;
  req.method = h2::verb::get;
  req.url2.SetHost(std::string(bucket) + ".s3.amazonaws.com");
  req.url2.SetPath(absl::StrCat("/", key));
  req.headers.emplace("host", std::string(bucket) + ".s3.amazonaws.com");
  req.headers.emplace("range", range);

  AwsResult<Response> resp = Send(&req);
  if (!resp) {
    return nonstd::make_unexpected(resp.error());
  }

  GetObjectResult result;
  result.body = std::move(resp->body);

  if (resp->headers["content-range"] == "") {
    result.object_size = result.body.size();
  } else {
    std::vector<std::string_view> parts = absl::StrSplit(resp->headers["content-range"], "/");
    if (parts.size() < 2) {
      LOG(ERROR) << "aws: s3 read file: failed to parse file size: range="
                 << resp->headers["content-range"];
      // TODO return std::make_error_code(std::errc::io_error);
    }
    if (!absl::SimpleAtoi(parts[1], &result.object_size)) {
      LOG(ERROR) << "aws: s3 read file: failed to parse file size: range="
                 << resp->headers["content-range"];
      // TODO return std::make_error_code(std::errc::io_error);
    }
  }

  return result;
}

AwsResult<std::string> Client::CreateMultipartUpload(std::string_view bucket,
                                                     std::string_view key) {
  Request req;
  req.method = h2::verb::post;
  req.url2.SetHost(std::string(bucket) + ".s3.amazonaws.com");
  req.url2.SetPath(absl::StrCat("/", key));
  req.url2.AddParameter("uploads", "");
  req.headers.emplace("host", std::string(bucket) + ".s3.amazonaws.com");

  AwsResult<Response> resp = Send(&req);
  if (!resp) {
    return nonstd::make_unexpected(resp.error());
  }

  CreateMultipartUploadResult::Parse(resp->body);

  AwsResult<CreateMultipartUploadResult> result = CreateMultipartUploadResult::Parse(resp->body);
  if (!result) {
    return nonstd::make_unexpected(result.error());
  }

  VLOG(1) << "aws: created multipart upload; upload_id=" << result->upload_id;

  return result->upload_id;
}

AwsResult<std::string> Client::UploadPart(std::string_view bucket, std::string_view key,
                                          int part_number, std::string_view upload_id,
                                          std::string_view part) {
  Request req;
  req.method = h2::verb::put;
  req.url2.SetHost(std::string(bucket) + ".s3.amazonaws.com");
  req.url2.SetPath(absl::StrCat("/", key));
  req.url2.AddParameter("partNumber", absl::StrFormat("%d", part_number));
  req.url2.AddParameter("uploadId", std::string(upload_id));
  req.headers.emplace("host", std::string(bucket) + ".s3.amazonaws.com");
  req.body = part;
  req.headers.emplace("content-length", absl::StrFormat("%d", part.size()));

  AwsResult<Response> resp = Send(&req);
  if (!resp) {
    return nonstd::make_unexpected(resp.error());
  }

  return resp->headers["ETag"];
}

AwsResult<void> Client::CompleteMultipartUpload(std::string_view bucket, std::string_view key,
                                                std::string_view upload_id,
                                                const std::vector<std::string>& parts) {
}

}  // namespace s3
}  // namespace awsv2
}  // namespace util
