// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/awsv2/s3/client.h"

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

AwsResult<CreateMultipartUploadResult> CreateMultipartUploadResult::Parse(std::string_view s) {
  pugi::xml_document doc;
  const pugi::xml_parse_result xml_result = doc.load_buffer(s.data(), s.size());
  if (!xml_result) {
    return nonstd::make_unexpected(AwsError{AwsErrorType::INVALID_RESPONSE,
                                            "parse create multipart upload response: invalid xml",
                                            false});
  }

  const pugi::xml_node root = doc.child("InitiateMultipartUploadResult");
  if (root.type() != pugi::node_element) {
    return nonstd::make_unexpected(AwsError{
        AwsErrorType::INVALID_RESPONSE,
        "parse create multipart upload response: InitiateMultipartUploadResult not found", false});
  }

  CreateMultipartUploadResult result;
  result.upload_id = root.child("UploadId").text().get();
  return result;
}

AwsResult<CompleteMultipartUploadResult> CompleteMultipartUploadResult::Parse(std::string_view s) {
  pugi::xml_document doc;
  const pugi::xml_parse_result xml_result = doc.load_buffer(s.data(), s.size());
  if (!xml_result) {
    return nonstd::make_unexpected(AwsError{AwsErrorType::INVALID_RESPONSE,
                                            "parse complete multipart upload response: invalid xml",
                                            false});
  }

  const pugi::xml_node root = doc.child("CompleteMultipartUploadResult");
  if (root.type() != pugi::node_element) {
    return nonstd::make_unexpected(AwsError{
        AwsErrorType::INVALID_RESPONSE,
        "parse complete multipart upload response: CompleteMultipartUploadResult not found",
        false});
  }

  CompleteMultipartUploadResult result;
  result.etag = root.child("ETag").text().get();
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

AwsResult<GetObjectResult> Client::GetObject(std::string_view bucket, std::string_view key,
                                             std::string_view range) {
  Request req;
  req.method = h2::verb::get;
  req.url.SetHost(std::string(bucket) + ".s3.amazonaws.com");
  req.url.SetPath(absl::StrCat("/", key));
  req.headers.emplace("range", range);

  AwsResult<Response> resp = Send(req);
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
      return nonstd::make_unexpected(AwsError{AwsErrorType::INVALID_RESPONSE,
                                              "parse get object response: invalid range", false});
    }
    if (!absl::SimpleAtoi(parts[1], &result.object_size)) {
      return nonstd::make_unexpected(AwsError{AwsErrorType::INVALID_RESPONSE,
                                              "parse get object response: invalid range", false});
    }
  }

  return result;
}

AwsResult<std::string> Client::CreateMultipartUpload(std::string_view bucket,
                                                     std::string_view key) {
  Request req;
  req.method = h2::verb::post;
  req.url.SetHost(std::string(bucket) + ".s3.amazonaws.com");
  req.url.SetPath(absl::StrCat("/", key));
  req.url.AddParam("uploads", "");

  AwsResult<Response> resp = Send(req);
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
  req.url.SetHost(std::string(bucket) + ".s3.amazonaws.com");
  req.url.SetPath(absl::StrCat("/", key));
  req.url.AddParam("partNumber", absl::StrFormat("%d", part_number));
  req.url.AddParam("uploadId", std::string(upload_id));
  req.body = part;
  req.headers.emplace("content-length", absl::StrFormat("%d", part.size()));

  AwsResult<Response> resp = Send(req);
  if (!resp) {
    return nonstd::make_unexpected(resp.error());
  }

  return resp->headers["ETag"];
}

AwsResult<std::string> Client::CompleteMultipartUpload(std::string_view bucket,
                                                       std::string_view key,
                                                       std::string_view upload_id,
                                                       const std::vector<std::string>& parts) {
  std::stringstream ss;
  ss << "<?xml version=\"1.0\"?>";
  ss << "<CompleteMultipartUpload xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">";
  for (size_t i = 0; i != parts.size(); i++) {
    const int part_number = i + 1;
    const std::string& etag = parts[i];
    ss << "<Part>";
    ss << "<ETag>\"" << etag << "\"</ETag>";
    ss << "<PartNumber>" << part_number << "</PartNumber>";
    ss << "</Part>";
  }
  ss << "</CompleteMultipartUpload>";
  const std::string body = ss.str();

  Request req;
  req.method = h2::verb::post;
  req.url.SetHost(std::string(bucket) + ".s3.amazonaws.com");
  req.url.SetPath(absl::StrCat("/", key));
  req.url.AddParam("uploadId", std::string(upload_id));
  req.body = body;
  req.headers.emplace("content-length", absl::StrFormat("%d", body.size()));

  AwsResult<Response> resp = Send(req);
  if (!resp) {
    return nonstd::make_unexpected(resp.error());
  }

  AwsResult<CompleteMultipartUploadResult> result =
      CompleteMultipartUploadResult::Parse(resp->body);
  if (!result) {
    return nonstd::make_unexpected(result.error());
  }

  return result->etag;
}

}  // namespace s3
}  // namespace awsv2
}  // namespace util
