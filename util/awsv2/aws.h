// Copyright 2023, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#pragma once

#include "io/io.h"

namespace util {
namespace awsv2 {

struct AwsError {};

template <typename T> using AwsResult = io::Result<T, AwsError>;

}  // namespace awsv2
}  // namespace util
