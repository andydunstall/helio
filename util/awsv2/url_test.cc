// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/awsv2/url.h"

#include "base/gtest.h"

namespace util {
namespace awsv2 {

class UrlTest : public ::testing::Test {};

TEST_F(UrlTest, Scheme) {
  EXPECT_EQ("http", ToString(Scheme::HTTP));
  EXPECT_EQ("https", ToString(Scheme::HTTPS));
  EXPECT_EQ(Scheme::HTTP, FromString("http"));
  EXPECT_EQ(Scheme::HTTPS, FromString("https"));
}

TEST_F(UrlTest, SetScheme) {
  Url url;

  // HTTP.
  url.SetScheme(Scheme::HTTP);
  EXPECT_EQ(Scheme::HTTP, url.scheme());
  EXPECT_EQ(kHttpPort, url.port());

  // HTTPS.
  url.SetScheme(Scheme::HTTPS);
  EXPECT_EQ(Scheme::HTTPS, url.scheme());
  EXPECT_EQ(kHttpsPort, url.port());

  // Don't override custom port.
  url.SetPort(9000);
  url.SetScheme(Scheme::HTTP);
  EXPECT_EQ(Scheme::HTTP, url.scheme());
  EXPECT_EQ(9000, url.port());
}

TEST_F(UrlTest, SetHost) {
  Url url;

  // Default port.
  url.SetHost("localhost");
  EXPECT_EQ("localhost", url.host());
  EXPECT_EQ(kHttpsPort, url.port());

  // Default custom port.
  url.SetHost("localhost:9000");
  EXPECT_EQ("localhost", url.host());
  EXPECT_EQ(9000, url.port());
}

TEST_F(UrlTest, SetPath) {
  Url url;

  url.SetPath("/");
  EXPECT_EQ("", url.path());
  url.SetPath("///");
  EXPECT_EQ("", url.path());

  url.SetPath("/foo/bar/car/");
  EXPECT_EQ("foo/bar/car", url.path());

  // URL encode.
  url.SetPath("/foo!/dump-2023-10-26T08:37:15-0001.dfs");
  EXPECT_EQ("foo%21/dump-2023-10-26T08%3A37%3A15-0001.dfs", url.path());
}

TEST_F(UrlTest, QueryString) {
  Url url;

  EXPECT_EQ("", url.QueryString());

  url.AddParam("foo", "bar");
  EXPECT_EQ("foo=bar", url.QueryString());

  // URL encode.
  url.AddParam("marker", "dump-2023-10-26T08:37:15-0001.dfs");
  EXPECT_EQ("foo=bar&marker=dump-2023-10-26T08%3A37%3A15-0001.dfs", url.QueryString());

  // Ordered.
  url.AddParam("a", "%b%");
  EXPECT_EQ("a=%25b%25&foo=bar&marker=dump-2023-10-26T08%3A37%3A15-0001.dfs", url.QueryString());
}

TEST_F(UrlTest, ToString) {
  Url url;

  url.SetScheme(Scheme::HTTP);
  url.SetHost("s3.amazonaws.com");
  url.SetPath("/foo:bar!");
  url.AddParam("a", "b!");
  EXPECT_EQ("http://s3.amazonaws.com/foo%3Abar%21?a=b%21", url.ToString());

  url.SetScheme(Scheme::HTTPS);
  url.SetHost("localhost:9000");
  EXPECT_EQ("https://localhost:9000/foo%3Abar%21?a=b%21", url.ToString());
}

}  // namespace awsv2
}  // namespace util
