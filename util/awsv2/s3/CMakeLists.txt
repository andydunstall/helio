add_library(s3_lib client.cc)

    cxx_link(s3_lib awsv2_lib)
