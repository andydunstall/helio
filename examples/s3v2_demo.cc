// Copyright 2022, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include <memory>

#include <absl/flags/flag.h>
#include <absl/strings/str_join.h>

#include "base/init.h"
#include "util/cloud/aws/session.h"
#include "util/cloud/aws/s3.h"
#include "util/fibers/pool.h"
#include "util/proactor_pool.h"
#include "util/http/http_client.h"

ABSL_FLAG(std::string, aws_access_key_id, "", "AWS access key ID");
ABSL_FLAG(std::string, aws_secret_access_key, "", "AWS secret access key");

int main(int argc, char* argv[]) {
  MainInitGuard guard(&argc, &argv);

  std::unique_ptr<util::fb2::Pool> pp;
  pp.reset(util::fb2::Pool::IOUring(256));
  pp->Run();

  util::cloud::aws::Session session{
    absl::GetFlag(FLAGS_aws_access_key_id),
    absl::GetFlag(FLAGS_aws_secret_access_key),
  };

  util::ProactorBase* proactor = pp->GetNextProactor();
  util::http::Client http_client{proactor};
  http_client.set_connect_timeout_ms(2000);

  util::cloud::aws::S3 s3{&session, &http_client};
  io::Result<std::vector<std::string>> buckets = proactor->Await([&] {
    // std::error_code ec = http_client.Connect("s3.amazonaws.com", "80");
    std::error_code ec = http_client.Connect("localhost", "9000");
    CHECK(!ec) << "failed to connect to s3: " << ec.message();
    return s3.ListBuckets();
  });
  if (!buckets) {
    LOG(ERROR) << "failed to list buckets: " << buckets.error();
    return 1;
  }
  if (buckets->size() == 0) {
    LOG(INFO) << "no buckets found";
    return 0;
  }

  LOG(INFO) << "buckets: " << absl::StrJoin(*buckets, ",");
}
