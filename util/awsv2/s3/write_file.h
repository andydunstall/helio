// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#pragma once

#include "io/file.h"
#include "io/io.h"
#include "util/awsv2/s3/client.h"

namespace util {
namespace awsv2 {
namespace s3 {

constexpr size_t kDefaultPartSize = 1ULL << 23;  // 8MB.

class WriteFile : public io::WriteFile {
 public:
  // Writes bytes to the S3 object. This will either buffer internally or
  // write a part to S3.
  io::Result<size_t> WriteSome(const iovec* v, uint32_t len) override {
    return 0;
  }

  // Closes the object and completes the multipart upload. Therefore the object
  // will not be uploaded unless Close is called.
  std::error_code Close() override {
    return std::error_code{};
  }

  static io::Result<WriteFile> Open(const std::string& bucket, const std::string& key,
                                    std::shared_ptr<Client> client,
                                    size_t part_size = kDefaultPartSize) {
    return nonstd::make_unexpected(std::make_error_code(std::errc::io_error));
  }
};

}  // namespace s3
}  // namespace awsv2
}  // namespace util
