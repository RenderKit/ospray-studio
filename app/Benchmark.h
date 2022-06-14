// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifndef USE_BENCHMARK
#error Benchmark.h requires USE_BENCHMARK to be set (probably a developer error?)
#endif

#include "Batch.h"
#include <benchmark/benchmark.h>

class BenchmarkContext : public BatchContext
{
 public:
  BenchmarkContext(StudioCommon &studioCommon)
    : BatchContext(studioCommon)
    {}
  ~BenchmarkContext() {}

  void start() override;
  void renderFrame() override;
};
