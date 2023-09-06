find_package(LibXml2)

add_library(awsv2_lib session.cc signerv4.cc s3.cc)

cxx_test(signerv4_test awsv2_lib LABELS CI)

if(TARGET LibXml2::LibXml2)
  cxx_link(awsv2_lib base OpenSSL::Crypto LibXml2::LibXml2 TRDP::rapidjson http_utils http_client_lib io absl::log absl::check)
else()
  Message(WARNING "LibXml2 not found, if you need aws_lib, install LibXml2 with 'sudo apt install libxml2-dev'")
endif()
