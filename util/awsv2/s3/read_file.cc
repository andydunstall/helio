// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/awsv2/s3/read_file.h"

#include "base/logging.h"

namespace util {
namespace awsv2 {
namespace s3 {

ReadFile::ReadFile(const std::string& bucket, const std::string& key,
                   std::shared_ptr<Client> client, size_t chunk_size)
    : bucket_{bucket}, key_{key}, client_{client}, buf_{chunk_size} {
}

io::Result<size_t> ReadFile::Read(size_t offset, const iovec* v, uint32_t len) {
  size_t read_n = 0;
  for (uint32_t i = 0; i != len; i++) {
    io::Result<size_t> n = Read(v[i]);
    if (!n) {
      return n;
    }
    read_n += *n;
  }

  VLOG(2) << "aws: s3 read file: read=" << read_n << "; file_read=" << file_read_
          << "; file_size=" << file_size_;

  if (read_n == 0) {
    VLOG(2) << "aws: s3 read file: read complete; file_size=" << file_size_;
  }

  return read_n;
}

std::error_code ReadFile::Close() {
  return std::error_code{};
}

size_t ReadFile::Size() const {
  return file_size_;
}

int ReadFile::Handle() const {
  return 0;
}

io::Result<size_t> ReadFile::Read(iovec v) {
  size_t read_n = 0;
  while (true) {
    // If we're read the whole file we're done.
    if (file_size_ > 0 && file_read_ == file_size_) {
      return read_n;
    }

    // Take as many bytes from the downloaded chunk as possible.
    size_t n = v.iov_len - read_n;
    if (n == 0) {
      return read_n;
    }
    if (n > buf_.InputLen()) {
      n = buf_.InputLen();
    }
    uint8_t* b = reinterpret_cast<uint8_t*>(v.iov_base);
    memcpy(b + read_n, buf_.InputBuffer().data(), n);
    buf_.ConsumeInput(n);
    read_n += n;
    file_read_ += n;

    // If we don't have sufficient bytes to fill v and haven't read the whole
    // file, read again.
    if (read_n < v.iov_len) {
      std::error_code ec = DownloadChunk();
      if (ec) {
        return nonstd::make_unexpected(ec);
      }
    }
  }
}

std::error_code ReadFile::DownloadChunk() {
  // If we're read the whole file we're done.
  if (file_size_ > 0 && file_read_ == file_size_) {
    return std::error_code{};
  }

  AwsResult<GetObjectResult> chunk = client_->GetObject(bucket_, key_, NextByteRange());
  if (!chunk) {
    LOG(ERROR) << "failed to download object: " << chunk.error().ToString();
    return std::make_error_code(std::errc::io_error);
  }

  VLOG(2) << "aws: s3 read file: downloaded chunk: length=" << chunk->body.size();

  // Verify we've read the full chunk before downloading another to ensure
  // we have capacity.
  CHECK_EQ(buf_.InputLen(), 0U);

  io::MutableBytes append_buf = buf_.AppendBuffer();
  memcpy(append_buf.data(), chunk->body.data(), chunk->body.size());
  buf_.CommitWrite(chunk->body.size());

  // If this is the first download, read the file size.
  if (file_size_ == 0) {
    file_size_ = chunk->object_size;
  }
  return std::error_code{};
}

std::string ReadFile::NextByteRange() const {
  return absl::StrFormat("bytes=%d-%d", file_read_, file_read_ + buf_.Capacity() - 1);
}

}  // namespace s3
}  // namespace awsv2
}  // namespace util
