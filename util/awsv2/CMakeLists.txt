add_library(aws_lib aws.cc)

cxx_link(aws_lib io)

add_subdirectory(s3)
