add_library(s3_lib client.cc)

cxx_link(s3_lib aws_lib TRDP::pugixml)

cxx_test(client_test s3_lib LABELS CI)
