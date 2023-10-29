add_library(s3_lib client.cc read_file.cc write_file.cc)

cxx_link(s3_lib aws_lib io TRDP::pugixml)

cxx_test(client_test s3_lib LABELS CI)
