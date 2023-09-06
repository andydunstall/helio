// Copyright 2022, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/cloud/aws/s3.h"

#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <glog/logging.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

namespace util {
namespace cloud {
namespace aws {

namespace bhttp = boost::beast::http;

namespace {

inline xmlDocPtr XmlRead(std::string_view xml) {
  return xmlReadMemory(xml.data(), xml.size(), NULL, NULL, XML_PARSE_COMPACT | XML_PARSE_NOBLANKS);
}

inline const char* as_char(const xmlChar* var) {
  return reinterpret_cast<const char*>(var);
}

std::vector<std::string> ParseXmlListBuckets(std::string_view xml_resp) {
  xmlDocPtr doc = XmlRead(xml_resp);
  CHECK(doc);

  xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);

  auto register_res = xmlXPathRegisterNs(xpathCtx, BAD_CAST "NS",
                                         BAD_CAST "http://s3.amazonaws.com/doc/2006-03-01/");
  CHECK_EQ(register_res, 0);

  xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(
      BAD_CAST "/NS:ListAllMyBucketsResult/NS:Buckets/NS:Bucket/NS:Name", xpathCtx);
  CHECK(xpathObj);
  xmlNodeSetPtr nodes = xpathObj->nodesetval;
  std::vector<std::string> res;
  if (nodes) {
    int size = nodes->nodeNr;
    for (int i = 0; i < size; ++i) {
      xmlNodePtr cur = nodes->nodeTab[i];
      CHECK_EQ(XML_ELEMENT_NODE, cur->type);
      CHECK(cur->ns);
      CHECK(nullptr == cur->content);

      if (cur->children && cur->last == cur->children && cur->children->type == XML_TEXT_NODE) {
        CHECK(cur->children->content);
        res.push_back(as_char(cur->children->content));
      }
    }
  }

  xmlXPathFreeObject(xpathObj);
  xmlXPathFreeContext(xpathCtx);
  xmlFreeDoc(doc);

  return res;
}

}  // namespace

S3::S3(Session* session, util::http::Client* http_client)
    : session_{session}, http_client_{http_client} {}

io::Result<std::vector<std::string>> S3::ListBuckets() {
  bhttp::request<bhttp::empty_body> req{bhttp::verb::get, "/", 11};
  req.set(bhttp::field::host, http_client_->host());

  // session_->Signer().Sign(&req, "s3", "us-east-1", absl::FromTimeT(0));
  session_->Signer().Sign(&req, "s3", "us-east-1");

  DVLOG(1) << "list buckets request: " << req;

  std::error_code ec = http_client_->Send(req);
  if (ec) {
    std::cout << "failed to send request " << ec << std::endl;
    // TODO
  }

  bhttp::response<bhttp::string_body> resp;
  ec = http_client_->Recv(&resp);
  if (ec) {
    // TODO
  }

  DVLOG(1) << "list buckets response: " << resp;

  return ParseXmlListBuckets(resp.body());
}

}  // namespace aws
}  // namespace cloud
}  // namespace util
