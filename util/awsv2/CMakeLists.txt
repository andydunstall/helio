add_library(aws_lib credentials_provider.cc)

cxx_link(aws_lib base)

cxx_test(credentials_provider_test aws_lib LABELS CI)
