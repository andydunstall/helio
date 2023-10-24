// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#pragma once

#include "base/io_buf.h"
#include "io/file.h"
#include "io/io.h"
#include "util/awsv2/s3/client.h"

namespace util {
namespace awsv2 {
namespace s3 {

constexpr size_t kDefaultChunkSize = 1ULL << 23;  // 8MB.

class ReadFile final : public io::ReadonlyFile {
 public:
  ReadFile(const std::string& bucket, const std::string& key, std::shared_ptr<Client> client,
           size_t chunk_size = kDefaultChunkSize) {
  }

  io::Result<size_t> Read(size_t offset, const iovec* v, uint32_t len) override {
    return 0;
  }

  std::error_code Close() override {
    return std::error_code{};
  }

  size_t Size() const override {
    return 0;
  }

  int Handle() const override {
    return 0;
  }
};

}  // namespace s3
}  // namespace awsv2
}  // namespace util
