// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "base/flags.h"
#include "base/init.h"
#include "base/logging.h"
#include "util/awsv2/s3/client.h"
#include "util/awsv2/s3/read_file.h"
#include "util/awsv2/s3/write_file.h"
#include "util/fibers/pool.h"

ABSL_FLAG(std::string, cmd, "list-buckets", "Command to run");
ABSL_FLAG(std::string, bucket, "", "S3 bucket name");
ABSL_FLAG(std::string, key, "", "S3 file key");
ABSL_FLAG(std::string, endpoint, "", "S3 endpoint");
ABSL_FLAG(size_t, upload_size, 100 << 20, "Upload file size");
ABSL_FLAG(size_t, chunk_size, 1024, "File chunk size");
ABSL_FLAG(bool, epoll, false, "Whether to use epoll instead of io_uring");
ABSL_FLAG(bool, https, true, "Whether to use HTTPS");
ABSL_FLAG(bool, ec2_metadata, false, "Whether to use EC2 metadata");

void ListBuckets() {
  util::awsv2::s3::Client client{absl::GetFlag(FLAGS_endpoint), absl::GetFlag(FLAGS_https),
                                 absl::GetFlag(FLAGS_ec2_metadata), true};
  util::awsv2::AwsResult<std::vector<std::string>> buckets = client.ListBuckets();
  if (!buckets) {
    // TODO ...
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
  util::awsv2::s3::Client client{absl::GetFlag(FLAGS_endpoint), absl::GetFlag(FLAGS_https),
                                 absl::GetFlag(FLAGS_ec2_metadata), true};
  util::awsv2::AwsResult<std::vector<std::string>> objects = client.ListObjects(absl::GetFlag(FLAGS_bucket));
  if (!objects) {
    // TODO ...
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
    } else {
      LOG(ERROR) << "unknown command: " << cmd;
    }
  });

  pp->Stop();
  return 0;
}
