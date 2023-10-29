// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#pragma once

#include <openssl/ssl.h>

#include <string>

#include "io/io.h"
#include "util/awsv2/aws.h"
#include "util/awsv2/url.h"
#include "util/fiber_socket_base.h"
#include "util/fibers/proactor_base.h"
#include "util/http/http_client.h"

namespace util {
namespace awsv2 {

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
  AwsResult<Response> Send(const Request& req);

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

  ProactorBase* proactor_;

  SSL_CTX* ctx_;

  std::optional<Connection> conn_;
};

}  // namespace awsv2
}  // namespace util
