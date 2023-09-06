#include "base/gtest.h"
#include "util/cloud/aws/signerv4.h"

#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

namespace util {
namespace cloud {
namespace aws {

namespace bhttp = boost::beast::http;

TEST(Foo, Foo) {
  Credentials creds;
  creds.access_key_id = "abc";
  creds.secret_access_key = "123";

  util::cloud::aws::SignerV4 signer{creds};

  bhttp::request<bhttp::empty_body> req{bhttp::verb::get, "/", 11};
  req.set(bhttp::field::host, "s3.us-east-1.amazonaws.com");

  signer.Sign(&req, "s3", "us-east-1", absl::FromTimeT(0));

  // session_->Signer().Sign(req, time(NULL));

}

}  // namespace aws
}  // namespace cloud
}  // namespace util
