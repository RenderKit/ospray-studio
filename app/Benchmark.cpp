// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Benchmark.h"

void BenchmarkContext::start() {
  ::benchmark::Initialize(&studioCommon.argc, (char **)(studioCommon.argv));
  BatchContext::start();
}

void BenchmarkContext::renderFrame() {
  ::benchmark::ClearRegisteredBenchmarks();
  ::benchmark::RegisterBenchmark("OSPRay Studio Benchmark", [&](::benchmark::State &state) {
    for (auto _ : state) {
      state.PauseTiming();
      frame->resetAccumulation();
      state.ResumeTiming();
      frame->immediatelyWait = true;
      frame->startNewFrame();
    }
  })->Unit(::benchmark::kMillisecond)->MinTime(60.0);
  ::benchmark::RunSpecifiedBenchmarks();
}