// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#pragma once

#include <openssl/ssl.h>

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/url/url.hpp>
#include <map>
#include <string>

#include "io/io.h"
#include "util/awsv2/url.h"
#include "util/fiber_socket_base.h"
#include "util/fibers/proactor_base.h"
#include "util/http/http_client.h"

namespace util {
namespace awsv2 {

namespace h2 = boost::beast::http;

struct Request {
  h2::verb method;
  Url url2;
  std::map<std::string, std::string> headers;
  std::string_view body;
};

struct Response {
  h2::status status;
  std::string body;
};

// TODO(andydunstall): Use AwsError.
enum class HttpError {
  OK,
  // Failed to connect to the host.
  CONNECT,
  // Failed to send the request or receive a response.
  REQUEST,
};

template <typename T> using HttpResult = io::Result<T, HttpError>;

// HttpClient is a HTTP/HTTPS client.
//
// It should only be used by a single fiber/proactor. It is not thread safe or
// fiber safe.
//
// Since the client is only to be used by a single proactor, and isn't expected
// to change connected hosts often, it only caches the most recent connection.
// If a request is made to another host, or uses another protocol protocol, the
// cached connection is closed and a new connection is created.
//
// The client does NOT follow redirects.
class HttpClient {
 public:
  HttpClient();

  ~HttpClient();

  HttpClient(const HttpClient&) = delete;
  HttpClient& operator=(const HttpClient&) = delete;

  HttpClient(HttpClient&&) = delete;
  HttpClient& operator=(HttpClient&&) = delete;

  // Sends a HTTP request and returns the HTTP response.
  //
  // If the request does not receive a response, an error is returned. A
  // non-2xx response does not cause an error.
  //
  // Requests will not be retried.
  HttpResult<Response> Send(const Request& req);

 private:
  struct Connection {
    std::unique_ptr<http::Client> client;
    std::string host;
    uint16_t port;
    bool tls;
  };

  // Connects to the given host and port.
  //
  // If there is already a cached connection to the host and port, it is
  // returned, otherwise a new connection is created.
  bool Connect(std::string_view host, uint16_t port, bool tls);

  HttpResult<boost::asio::ip::address> Resolve(std::string_view host) const;

  ProactorBase* proactor_;

  SSL_CTX* ctx_;

  boost::beast::flat_buffer buf_;

  std::optional<Connection> conn_;
};

}  // namespace awsv2
}  // namespace util
