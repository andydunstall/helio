// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#include "util/cloud/session.h"

#include <cstdlib>

#include "base/gtest.h"
#include "io/io.h"

namespace util {
namespace cloud {

namespace {

class TempFile {
 public:
  ~TempFile();

  TempFile(TempFile&) = delete;
  TempFile& operator=(TempFile&) = delete;

  TempFile(TempFile&&);
  TempFile& operator=(TempFile&&);

  std::string path() const {
    return path_;
  }

  std::error_code Write(std::string_view s);

  static io::Result<TempFile> Open();

 private:
  TempFile(int fd, const std::string& path);

  int fd_;

  std::string path_;
};

TempFile::TempFile(int fd, const std::string& path) : fd_{fd}, path_{path} {
}

TempFile::~TempFile() {
  if (fd_ != -1) {
    close(fd_);
    unlink(path_.c_str());
  }
}

TempFile::TempFile(TempFile&& f) {
  fd_ = f.fd_;
  path_ = f.path_;
  // Set to -1 to stop closing.
  f.fd_ = -1;
}

TempFile& TempFile::operator=(TempFile&& f) {
  fd_ = f.fd_;
  path_ = f.path_;
  // Set to -1 to stop closing.
  f.fd_ = -1;
  return *this;
}

std::error_code TempFile::Write(std::string_view s) {
  size_t written = 0;
  while (written < s.size()) {
    ssize_t n = write(fd_, s.data() + written, s.size() - written);
    if (n == -1) {
      return std::error_code{errno, std::system_category()};
    }
    written += n;
  }
  return std::error_code{};
}

io::Result<TempFile> TempFile::Open() {
  char filename[] = "/tmp/dragonfly.XXXXXX";
  int fd = mkstemp(filename);
  if (fd == -1) {
    return nonstd::make_unexpected(std::error_code{errno, std::system_category()});
  }
  return TempFile{fd, filename};
}

}  // namespace

class EnvProviderTest : public ::testing::Test {};

TEST_F(EnvProviderTest, Load) {
  ASSERT_EQ(0, setenv("AWS_ACCESS_KEY_ID", "access", 1));
  ASSERT_EQ(0, setenv("AWS_SECRET_ACCESS_KEY", "secret", 1));
  ASSERT_EQ(0, setenv("AWS_SESSION_TOKEN", "session", 1));
  ASSERT_EQ(0, setenv("AWS_REGION", "region", 1));

  EnvProvider provider;
  std::optional<Session> session = provider.Load();
  ASSERT_TRUE(session);
  EXPECT_EQ("access", session->credentials.access_key_id);
  EXPECT_EQ("secret", session->credentials.secret_access_key);
  EXPECT_EQ("session", session->credentials.session_token);
  EXPECT_EQ("region", session->region);
}

TEST_F(EnvProviderTest, LoadNoAccessKey) {
  ASSERT_EQ(0, unsetenv("AWS_ACCESS_KEY_ID"));
  ASSERT_EQ(0, setenv("AWS_SECRET_ACCESS_KEY", "secret", 1));
  ASSERT_EQ(0, unsetenv("AWS_SESSION_TOKEN"));

  EnvProvider provider;
  std::optional<Session> session = provider.Load();
  ASSERT_FALSE(session);
}

TEST_F(EnvProviderTest, LoadNoSecretKey) {
  ASSERT_EQ(0, setenv("AWS_ACCESS_KEY_ID", "access", 1));
  ASSERT_EQ(0, unsetenv("AWS_SECRET_ACCESS_KEY"));

  EnvProvider provider;
  std::optional<Session> session = provider.Load();
  ASSERT_FALSE(session);
}

TEST_F(EnvProviderTest, AlternateNames) {
  ASSERT_EQ(0, setenv("AWS_ACCESS_KEY", "access", 1));
  ASSERT_EQ(0, setenv("AWS_SECRET_KEY", "secret", 1));
  ASSERT_EQ(0, setenv("AWS_DEFAULT_REGION", "region", 1));
  ASSERT_EQ(0, unsetenv("AWS_SESSION_TOKEN"));

  EnvProvider provider;
  std::optional<Session> session = provider.Load();
  ASSERT_TRUE(session);
  EXPECT_EQ("access", session->credentials.access_key_id);
  EXPECT_EQ("secret", session->credentials.secret_access_key);
  EXPECT_EQ("region", session->region);
}

class SharedConfigProviderTest : public ::testing::Test {};

TEST_F(SharedConfigProviderTest, LoadDefaultProfile) {
  io::Result<TempFile> creds_file = TempFile::Open();
  io::Result<TempFile> conf_file = TempFile::Open();
  ASSERT_TRUE(creds_file);
  ASSERT_TRUE(conf_file);

  ASSERT_EQ(0, setenv("AWS_SHARED_CREDENTIALS_FILE", creds_file->path().c_str(), 1));
  ASSERT_EQ(0, setenv("AWS_CONFIG_FILE", conf_file->path().c_str(), 1));
  ASSERT_EQ(0, unsetenv("AWS_PROFILE"));

  ASSERT_FALSE(creds_file->Write(R"%(
[default]
aws_access_key_id=access
aws_secret_access_key=secret
aws_session_token=session
)%"));
  ASSERT_FALSE(conf_file->Write(R"%(
[default]
region=region
)%"));

  SharedConfigProvider provider{};
  std::optional<Session> session = provider.Load();
  ASSERT_TRUE(session);
  EXPECT_EQ("access", session->credentials.access_key_id);
  EXPECT_EQ("secret", session->credentials.secret_access_key);
  EXPECT_EQ("session", session->credentials.session_token);
  EXPECT_EQ("region", session->region);
}

TEST_F(SharedConfigProviderTest, LoadCustomProfile) {
  io::Result<TempFile> creds_file = TempFile::Open();
  io::Result<TempFile> conf_file = TempFile::Open();
  ASSERT_TRUE(creds_file);
  ASSERT_TRUE(conf_file);

  ASSERT_EQ(0, setenv("AWS_SHARED_CREDENTIALS_FILE", creds_file->path().c_str(), 1));
  ASSERT_EQ(0, setenv("AWS_CONFIG_FILE", conf_file->path().c_str(), 1));
  ASSERT_EQ(0, setenv("AWS_PROFILE", "myprofile", 1));

  ASSERT_FALSE(creds_file->Write(R"%(
[default]
aws_access_key_id=nil
aws_secret_access_key=nil
aws_session_token=nil

[myprofile]
aws_access_key_id=access
aws_secret_access_key=secret
aws_session_token=session


)%"));
  ASSERT_FALSE(conf_file->Write(R"%(
[default]
region=nil

[myprofile]
region=region
)%"));

  SharedConfigProvider provider{};
  std::optional<Session> session = provider.Load();
  ASSERT_TRUE(session);
  EXPECT_EQ("access", session->credentials.access_key_id);
  EXPECT_EQ("secret", session->credentials.secret_access_key);
  EXPECT_EQ("session", session->credentials.session_token);
  EXPECT_EQ("region", session->region);
}

TEST_F(SharedConfigProviderTest, LoadRegionFromEnv) {
  io::Result<TempFile> creds_file = TempFile::Open();
  ASSERT_TRUE(creds_file);

  ASSERT_EQ(0, setenv("AWS_SHARED_CREDENTIALS_FILE", creds_file->path().c_str(), 1));
  ASSERT_EQ(0, setenv("AWS_REGION", "region", 1));
  ASSERT_EQ(0, unsetenv("AWS_PROFILE"));

  ASSERT_FALSE(creds_file->Write(R"%(
[default]
aws_access_key_id=access
aws_secret_access_key=secret
aws_session_token=session
)%"));

  SharedConfigProvider provider{};
  std::optional<Session> session = provider.Load();
  ASSERT_TRUE(session);
  EXPECT_EQ("access", session->credentials.access_key_id);
  EXPECT_EQ("secret", session->credentials.secret_access_key);
  EXPECT_EQ("session", session->credentials.session_token);
  EXPECT_EQ("region", session->region);
}

TEST_F(SharedConfigProviderTest, LoadInvalidConfig) {
  io::Result<TempFile> file = TempFile::Open();
  ASSERT_TRUE(file);

  ASSERT_EQ(0, setenv("AWS_SHARED_CREDENTIALS_FILE", file->path().c_str(), 1));

  ASSERT_FALSE(file->Write("invalid ini file"));

  SharedConfigProvider provider{};
  std::optional<Session> session = provider.Load();
  ASSERT_FALSE(session);
}

TEST_F(SharedConfigProviderTest, LoadFileNotFound) {
  ASSERT_EQ(0, setenv("AWS_SHARED_CREDENTIALS_FILE", "/tmp/dragonfly/myfile.ini", 1));

  SharedConfigProvider provider{};
  std::optional<Session> session = provider.Load();
  ASSERT_FALSE(session);
}

TEST_F(SharedConfigProviderTest, LoadFilePathNotFound) {
  ASSERT_EQ(0, unsetenv("AWS_SHARED_CREDENTIALS_FILE"));
  ASSERT_EQ(0, unsetenv("HOME"));

  SharedConfigProvider provider{};
  std::optional<Session> session = provider.Load();
  ASSERT_FALSE(session);
}

// TODO test Ec2RoleProvider

}  // namespace cloud
}  // namespace util
