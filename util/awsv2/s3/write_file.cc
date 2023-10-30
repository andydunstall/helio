// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/awsv2/s3/write_file.h"

#include "base/logging.h"

namespace util {
namespace awsv2 {
namespace s3 {

io::Result<size_t> WriteFile::WriteSome(const iovec* v, uint32_t len) {
  // Fill the pending buffer until we reach the part size, then flush and
  // keep writing.
  size_t total = 0;
  for (size_t i = 0; i < len; ++i) {
    const uint8_t* buf = reinterpret_cast<const uint8_t*>(v[i].iov_base);
    const size_t len = v[i].iov_len;

    size_t written = 0;
    while (written < len) {
      size_t n = len - written;
      if (n > buf_.size() - offset_) {
        // Limit to avoid exceeding the buffer size.
        n = buf_.size() - offset_;
      }
      memcpy(buf_.data() + offset_, buf + written, n);
      written += n;
      total += n;
      offset_ += n;

      if (buf_.size() == offset_) {
        std::error_code ec = Flush();
        if (ec) {
          return nonstd::make_unexpected(ec);
        }
      }
    }
  }

  return total;
}

std::error_code WriteFile::Close() {
  std::error_code ec = Flush();
  if (ec) {
    return ec;
  }

  AwsResult<std::string> result =
      client_->CompleteMultipartUpload(bucket_, key_, upload_id_, parts_);
  if (!result) {
    LOG(ERROR) << "failed to upload object: " << result.error().ToString();
    return std::make_error_code(std::errc::io_error);
  }

  return std::error_code{};
}

AwsResult<WriteFile> WriteFile::Open(const std::string& bucket, const std::string& key,
                                     std::shared_ptr<Client> client, size_t part_size) {
  AwsResult<std::string> upload_id = client->CreateMultipartUpload(bucket, key);
  if (!upload_id) {
    return nonstd::make_unexpected(upload_id.error());
  }
  return WriteFile{bucket, key, *upload_id, client, part_size};
}

WriteFile::WriteFile(const std::string& bucket, const std::string& key,
                     const std::string& upload_id, std::shared_ptr<Client> client, size_t part_size)
    : io::WriteFile{""}, bucket_{bucket}, key_{key}, upload_id_{upload_id}, client_{client},
      buf_(part_size) {
}

std::error_code WriteFile::Flush() {
  if (offset_ == 0) {
    return std::error_code{};
  }

  AwsResult<std::string> etag =
      client_->UploadPart(bucket_, key_, parts_.size() + 1, upload_id_,
                          std::string_view(reinterpret_cast<const char*>(buf_.data()), offset_));
  if (!etag) {
    LOG(ERROR) << "failed to upload object: " << etag.error().ToString();
    return std::make_error_code(std::errc::io_error);
  }

  parts_.push_back(*etag);
  offset_ = 0;
  return std::error_code{};
}

}  // namespace s3
}  // namespace awsv2
}  // namespace util
