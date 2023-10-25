// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <array>

#include "rkcommon/math/vec.h"
#include "sg/Node.h"

#ifdef USE_MPI
#include <mpi.h>
#endif

namespace ospray {
namespace sg {

using namespace rkcommon::math;

struct OSPSG_INTERFACE SgMPI
{
    int usingMpi = 0;
    int mpiRank = 0;
    int mpiWorldSize = 1;
};

extern OSPSG_INTERFACE SgMPI sgMPI;

inline void sgAssignMPI(int rank, int size)
{
#ifdef USE_MPI
  sgMPI.usingMpi = 1;
  sgMPI.mpiRank = rank;
  sgMPI.mpiWorldSize = size;
#endif
}

inline void sgInitializeMPI(int argc, const char *argv[])
{
#ifdef USE_MPI
    int mpiThreadCapability = 0;
    MPI_Init_thread(&argc, (char***)(&argv), MPI_THREAD_MULTIPLE, &mpiThreadCapability);
    if (mpiThreadCapability != MPI_THREAD_MULTIPLE
        && mpiThreadCapability != MPI_THREAD_SERIALIZED) {
      fprintf(stderr,
          "Fatal: OSPRay requires the MPI runtime to support thread "
          "multiple or thread serialized.\n");
      exit(1);
    }

    MPI_Initialized(&sgMPI.usingMpi);

    if (!sgMPI.usingMpi){
      fprintf(stderr,
      "Fatal: MPI was initialized, but an MPI_Initialized() disagrees.\n");
      exit(1);
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &sgMPI.mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &sgMPI.mpiWorldSize);
#else
    fprintf(stderr,
        "Fatal: sgInitializeMPI() was called, but MPI support was not "
        "compiled into OSPRay Studio. Recompile with USE_MPI set to ON.\n");
    exit(1);
#endif
}

inline int sgUsingMpi()
{
    return sgMPI.usingMpi;
}

inline int sgMpiRank()
{
    return sgMPI.mpiRank;
}

inline int sgMpiWorldSize()
{
    return sgMPI.mpiWorldSize;
}

inline void sgMpiBarrier()
{
#ifdef USE_MPI
    MPI_Barrier(MPI_COMM_WORLD);
#endif
}

inline bool compute_divisor(int x, int &divisor)
{
    //Find the first half of possible divisors
    // for x and return it. Used to recursively
    // split space in compute_grid. 
    const int upper = std::sqrt(x);
    for (int i = 2; i <= upper; ++i) {
        if (x % i == 0) {
            divisor = i;
            return true;
        }
    }
    return false;
}

inline vec3i compute_grid(int num)
{
    //find the most even grid partitioning possible
    //  by recursively splitting space into the
    //  number of given ranks
    vec3i grid(1);
    int axis = 0;
    int divisor = 0;
    while (compute_divisor(num, divisor)) {
        grid[axis] *= divisor;
        num /= divisor;
        axis = (axis + 1) % 3;
    }
    if (num != 1) {
        grid[axis] *= num;
    }
    return grid;
}

enum GhostFace { NEITHER_FACE = 0, POS_FACE = 1, NEG_FACE = 2 };

inline std::array<int, 3> compute_ghost_faces(const vec3i &brick_id, const vec3i &grid)
{    
    std::array<int, 3> faces = {NEITHER_FACE, NEITHER_FACE, NEITHER_FACE};
    for (size_t i = 0; i < 3; ++i) {
        if (brick_id[i] < grid[i] - 1) {
            faces[i] |= POS_FACE;
        }
        if (brick_id[i] > 0) {
            faces[i] |= NEG_FACE;
        }
    }
    return faces;
}

} // namespace sg
} // namespace ospray
