add_library(s3_lib client.cc)

cxx_link(s3_lib io aws_lib)
