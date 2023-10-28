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

// Reads files from S3.
//
// This downloads chunks of the file with the given chunk size, then Read
// consumes from the buffered chunk. Once a chunk has been read, it downloads
// another.
class ReadFile final : public io::ReadonlyFile {
 public:
  ReadFile(const std::string& bucket, const std::string& key, std::shared_ptr<Client> client,
           size_t chunk_size = kDefaultChunkSize);

  io::Result<size_t> Read(size_t offset, const iovec* v, uint32_t len) override;

  std::error_code Close() override;

  size_t Size() const override;

  int Handle() const override;

 private:
  io::Result<size_t> Read(iovec v);

  std::error_code DownloadChunk();

  std::string NextByteRange() const;

  std::string bucket_;

  std::string key_;

  std::shared_ptr<Client> client_;

  base::IoBuf buf_;

  // Total bytes in the file read.
  size_t file_read_ = 0;

  // Size of the target file to read. Set on first download.
  size_t file_size_ = 0;
};

}  // namespace s3
}  // namespace awsv2
}  // namespace util
