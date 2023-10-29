add_library(aws_lib aws.cc client.cc credentials_provider.cc http_client.cc url.cc v4_signer.cc)

cxx_link(aws_lib base http_client_lib TRDP::pugixml)

cxx_test(credentials_provider_test aws_lib LABELS CI)
cxx_test(url_test aws_lib LABELS CI)
cxx_test(v4_signer_test aws_lib LABELS CI)

add_subdirectory(s3)
