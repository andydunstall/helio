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
  io::Result<size_t> WriteSome(const iovec* v, uint32_t len) override;

  // Closes the object and completes the multipart upload. Therefore the object
  // will not be uploaded unless Close is called.
  std::error_code Close() override;

  static AwsResult<WriteFile> Open(const std::string& bucket, const std::string& key,
                                   std::shared_ptr<Client> client,
                                   size_t part_size = kDefaultPartSize);

 private:
  WriteFile(const std::string& bucket, const std::string& key, const std::string& upload_id,
            std::shared_ptr<Client> client, size_t part_size);

  // Uploads the data buffered in buf_. Note this must not be called until
  // there are at least 5MB bytes in buf_, unless it is the last upload.
  std::error_code Flush();

  std::string bucket_;

  std::string key_;

  std::string upload_id_;

  std::shared_ptr<Client> client_;

  // Etags of the uploaded parts.
  std::vector<std::string> parts_;

  // A buffer containing the pending bytes waiting to be uploaded. Only offset_
  // bytes have been written.
  std::vector<uint8_t> buf_;

  // Offset of the consumed bytes in buf_.
  size_t offset_ = 0;
};

}  // namespace s3
}  // namespace awsv2
}  // namespace util
