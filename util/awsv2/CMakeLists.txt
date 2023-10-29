add_library(aws_lib credentials_provider.cc url.cc)

cxx_link(aws_lib base)

cxx_test(credentials_provider_test aws_lib LABELS CI)
cxx_test(url_test aws_lib LABELS CI)
