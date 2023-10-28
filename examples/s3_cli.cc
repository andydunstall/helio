// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "base/flags.h"
#include "base/init.h"
#include "base/logging.h"
#include "util/awsv2/s3/client.h"
#include "util/fibers/pool.h"

ABSL_FLAG(std::string, cmd, "list-buckets", "Command to run");
ABSL_FLAG(std::string, bucket, "", "Target bucket");
ABSL_FLAG(std::string, prefix, "", "List objects prefix");
ABSL_FLAG(bool, epoll, false, "Whether to use epoll instead of io_uring");

void ListBuckets() {
  util::awsv2::s3::Client client{"us-east-1"};
  util::awsv2::AwsResult<std::vector<std::string>> buckets = client.ListBuckets();
  if (!buckets) {
    LOG(ERROR) << "failed to get buckets";
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

  util::awsv2::s3::Client client{"us-east-1"};
  util::awsv2::AwsResult<std::vector<std::string>> objects =
      client.ListObjects(absl::GetFlag(FLAGS_bucket), absl::GetFlag(FLAGS_prefix));
  if (!objects) {
    LOG(ERROR) << "failed to get objects";
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
