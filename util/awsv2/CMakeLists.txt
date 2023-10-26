add_library(aws_lib aws.cc client.cc credentials_provider.cc v4_signer.cc)

cxx_link(aws_lib io http_client_lib TRDP::pugixml)

add_subdirectory(s3)

cxx_test(credentials_provider_test aws_lib LABELS CI)
cxx_test(v4_signer_test aws_lib LABELS CI)
