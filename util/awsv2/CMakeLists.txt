add_library(aws_lib aws.cc client.cc credentials_provider.cc)

cxx_link(aws_lib io)

add_subdirectory(s3)

cxx_test(credentials_provider_test aws_lib LABELS CI)
