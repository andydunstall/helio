// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "base/flags.h"
#include "base/init.h"
#include "base/logging.h"
#include "util/awsv2/credentials_provider.h"
#include "util/awsv2/s3/client.h"
#include "util/awsv2/s3/read_file.h"
#include "util/awsv2/s3/write_file.h"
#include "util/fibers/pool.h"

ABSL_FLAG(std::string, cmd, "list-buckets", "Command to run");
ABSL_FLAG(std::string, bucket, "", "Target bucket");
ABSL_FLAG(std::string, prefix, "", "List objects prefix");
ABSL_FLAG(std::string, key, "", "Upload/download key");
ABSL_FLAG(size_t, upload_size, 100 << 20, "Upload file size");
ABSL_FLAG(size_t, chunk_size, 1024, "File chunk size");
ABSL_FLAG(bool, epoll, false, "Whether to use epoll instead of io_uring");
ABSL_FLAG(bool, https, true, "Whether to use HTTPS");
ABSL_FLAG(std::string, endpoint, "", "S3 endpoint");

void ListBuckets() {
  util::awsv2::Config config;
  config.region = "us-east-1";
  config.https = absl::GetFlag(FLAGS_https);
  config.endpoint = absl::GetFlag(FLAGS_endpoint);

  std::unique_ptr<util::awsv2::CredentialsProvider> credentials_provider =
      std::make_unique<util::awsv2::EnvironmentCredentialsProvider>();
  util::awsv2::s3::Client client{config, std::move(credentials_provider)};
  util::awsv2::AwsResult<std::vector<std::string>> buckets = client.ListBuckets();
  if (!buckets) {
    LOG(ERROR) << "failed to get buckets: " << buckets.error().ToString();
    return;
  }
  if (buckets->size() == 0) {
    std::cout << "no buckets found" << std::endl;
    return;
  }
  std::cout << "buckets:" << std::endl;
  for (const std::string& name : *buckets) {
    std::cout << "* " << name << std::endl;
  }
}

void ListObjects() {
  if (absl::GetFlag(FLAGS_bucket).empty()) {
    std::cout << "missing bucket" << std::endl;
    return;
  }

  util::awsv2::Config config;
  config.region = "us-east-1";
  config.https = absl::GetFlag(FLAGS_https);
  config.endpoint = absl::GetFlag(FLAGS_endpoint);

  std::unique_ptr<util::awsv2::CredentialsProvider> credentials_provider =
      std::make_unique<util::awsv2::EnvironmentCredentialsProvider>();
  util::awsv2::s3::Client client{config, std::move(credentials_provider)};
  util::awsv2::AwsResult<std::vector<std::string>> objects =
      client.ListObjects(absl::GetFlag(FLAGS_bucket), absl::GetFlag(FLAGS_prefix));
  if (!objects) {
    LOG(ERROR) << "failed to get objects: " << objects.error().ToString();
    return;
  }
  if (objects->size() == 0) {
    std::cout << "no objects found" << std::endl;
    return;
  }
  std::cout << "objects:" << std::endl;
  for (const std::string& name : *objects) {
    std::cout << "* " << name << std::endl;
  }
}

void Upload() {
  if (absl::GetFlag(FLAGS_bucket) == "") {
    LOG(ERROR) << "missing bucket name";
    return;
  }

  if (absl::GetFlag(FLAGS_key) == "") {
    LOG(ERROR) << "missing key";
    return;
  }

  util::awsv2::Config config;
  config.region = "us-east-1";
  config.https = absl::GetFlag(FLAGS_https);
  config.endpoint = absl::GetFlag(FLAGS_endpoint);

  std::unique_ptr<util::awsv2::CredentialsProvider> credentials_provider =
      std::make_unique<util::awsv2::EnvironmentCredentialsProvider>();
  std::shared_ptr<util::awsv2::s3::Client> client =
      std::make_shared<util::awsv2::s3::Client>(config, std::move(credentials_provider));
  util::awsv2::AwsResult<util::awsv2::s3::WriteFile> file = util::awsv2::s3::WriteFile::Open(
      absl::GetFlag(FLAGS_bucket), absl::GetFlag(FLAGS_key), client);
  if (!file) {
    LOG(ERROR) << "failed to open file: " << file.error().ToString();
    return;
  }

  size_t chunks = absl::GetFlag(FLAGS_upload_size) / absl::GetFlag(FLAGS_chunk_size);

  LOG(INFO) << "uploading s3 file; chunks=" << chunks
            << "; chunk_size=" << absl::GetFlag(FLAGS_chunk_size);

  std::vector<uint8_t> buf(absl::GetFlag(FLAGS_chunk_size), 0xff);
  for (size_t i = 0; i != chunks; i++) {
    std::error_code ec = file->Write(io::Bytes(buf.data(), buf.size()));
    if (ec) {
      LOG(ERROR) << "failed to write to s3";
      return;
    }
  }
  std::error_code ec = file->Close();
  if (ec) {
    LOG(ERROR) << "failed to close s3 write file";
    return;
  }
}

void Download() {
  if (absl::GetFlag(FLAGS_bucket) == "") {
    LOG(ERROR) << "missing bucket name";
    return;
  }

  if (absl::GetFlag(FLAGS_key) == "") {
    LOG(ERROR) << "missing key";
    return;
  }

  util::awsv2::Config config;
  config.region = "us-east-1";
  config.https = absl::GetFlag(FLAGS_https);
  config.endpoint = absl::GetFlag(FLAGS_endpoint);

  std::unique_ptr<util::awsv2::CredentialsProvider> credentials_provider =
      std::make_unique<util::awsv2::EnvironmentCredentialsProvider>();
  std::shared_ptr<util::awsv2::s3::Client> client =
      std::make_shared<util::awsv2::s3::Client>(config, std::move(credentials_provider));
  std::unique_ptr<io::ReadonlyFile> file = std::make_unique<util::awsv2::s3::ReadFile>(
      absl::GetFlag(FLAGS_bucket), absl::GetFlag(FLAGS_key), client);

  LOG(INFO) << "downloading s3 file";

  std::vector<uint8_t> buf(1024, 0);
  size_t read_n = 0;
  while (true) {
    io::Result<size_t> n = file->Read(read_n, io::MutableBytes(buf.data(), buf.size()));
    if (!n) {
      LOG(ERROR) << "failed to read from s3";
      return;
    }
    if (*n == 0) {
      LOG(INFO) << "finished download; read_n=" << read_n;
      return;
    }
    read_n += *n;
  }
}

int main(int argc, char* argv[]) {
  MainInitGuard guard(&argc, &argv);

  std::unique_ptr<util::ProactorPool> pp;

#ifdef __linux__
  if (absl::GetFlag(FLAGS_epoll)) {
    pp.reset(util::fb2::Pool::Epoll());
  } else {
    pp.reset(util::fb2::Pool::IOUring(256));
  }
#else
  pp.reset(util::fb2::Pool::Epoll());
#endif

  pp->Run();

  pp->GetNextProactor()->Await([&] {
    std::string cmd = absl::GetFlag(FLAGS_cmd);
    LOG(INFO) << "s3_cli; cmd=" << cmd;

    if (cmd == "list-buckets") {
      ListBuckets();
    } else if (cmd == "list-objects") {
      ListObjects();
    } else if (cmd == "upload") {
      Upload();
    } else if (cmd == "download") {
      Download();
    } else {
      LOG(ERROR) << "unknown command: " << cmd;
    }
  });

  pp->Stop();
  return 0;
}
