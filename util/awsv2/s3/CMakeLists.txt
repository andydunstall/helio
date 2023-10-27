add_library(s3_lib client.cc)

cxx_link(s3_lib aws_lib)
