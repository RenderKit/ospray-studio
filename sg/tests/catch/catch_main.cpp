// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include "ospray/ospray.h"

int main(int argc, char *argv[])
{
  ospInit(nullptr, nullptr);

  int result = Catch::Session().run(argc, argv);

  ospShutdown();

  return result;
}
