// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// WaveletSlices exists to create reproducible scenes with arbitrarily
// large polygon counts and controllable visual aspects.
// The algorithm is to create slice planes composed of triangles within
// a wavelet field. The relative size of triangles varies on each slice
// and the number of slices can be changed to alter uniformity and
// depth complexity

#include "Generator.h"
#include "WaveletField.h"
#include "sg/Mpi.h"

// std
#include <random>
// rkcommon
#include "rkcommon/tasking/parallel_for.h"

namespace ospray {
namespace sg {

struct WaveletSlices : public Generator
{
  WaveletSlices();
  ~WaveletSlices() override = default;

  void generateData() override;
};

OSP_REGISTER_SG_NODE_NAME(WaveletSlices, generator_wavelet_slices);

// WaveletVolume definitions ////////////////////////////////////////////////

WaveletSlices::WaveletSlices()
{
  auto &parameters = child("parameters");

  parameters.createChild("requestedTriangles", "long", 10000l);
  parameters.createChild("numSlices", "int", 20);
  parameters.createChild("sizeRatio", "float", 4.f);
  parameters.createChild("thresholdLow", "float", -6.f);
  parameters.createChild("thresholdHigh", "float", 6.f);
  parameters.createChild("actualTriangles", "long", 10000);
  parameters.child("actualTriangles").setReadOnly();

  createChild("xfm", "transform");
}

void WaveletSlices::generateData()
{
  auto &xfm = child("xfm");

  // Create voxel data

  auto &parameters = child("parameters");

  auto sizeRatio = parameters["sizeRatio"].valueAs<float>();
  if (sizeRatio <= 0) sizeRatio = 1;
  auto numSlices = parameters["numSlices"].valueAs<int>();
  if (numSlices < 1) numSlices = 1;
  long requestedTriangles;
  if (parameters["requestedTriangles"].valueIsType<int>()) {
    requestedTriangles = static_cast<long>(parameters["requestedTriangles"].valueAs<int>());
  } else if (parameters["requestedTriangles"].valueIsType<unsigned int>()) {
    requestedTriangles = static_cast<long>(parameters["requestedTriangles"].valueAs<unsigned int>());
  } else {
    requestedTriangles = parameters["requestedTriangles"].valueAs<long>();
  }

  if (requestedTriangles < numSlices*2) requestedTriangles = numSlices*2;

  auto thresholdLow = parameters["thresholdLow"].valueAs<float>();
  auto thresholdHigh = parameters["thresholdHigh"].valueAs<float>();


  //divy the Triangles across the slices while varying number of triangles so
  //that first slice has about sizeRatio times the number of the last slice.
  std::vector<float> ratios;
  float tr = 0.0;
  for (int s = 0; s < numSlices; s++) {
      float l = (s+1)/(float)numSlices;
      float r = (1-l)*sizeRatio + l*1;
      ratios.push_back(r);
      tr = tr + r;
  }
  long expectedTriangles = 0;
  long actualTriangles = 0;
  std::vector<vec3f> vertex;
  std::vector<vec4f> color;
  std::vector<vec3ui> index;

  int startSlice = 0;
  int endSlice = numSlices;

  //Distribute over slices.
  //TODO: not the best for load balancing, but simple.
  if (sgUsingMpi())
  {
    int mpiChunks = int(ceilf(numSlices / float(sgMpiWorldSize())));
    startSlice = sgMpiRank() * mpiChunks;
    endSlice = std::min<int>(numSlices, startSlice + mpiChunks);
  }

  float interslice = 2.0f/numSlices;
  for (int s = startSlice; s < endSlice; s++) {
      int tslice = requestedTriangles/tr * ratios[s];
      expectedTriangles = expectedTriangles + tslice;

      int gridSteps = sqrt(tslice/2); //quads
      //std::cerr << "slice " << s << " goal " << tslice << " gridSteps " << gridSteps <<  std::endl;

      for (int y = 0; y < gridSteps; ++y) {
          //std::cerr << y << std::endl;

          for (int z = 0; z < gridSteps; ++z) {
              vec3f corner0(
                          (s*128.f/numSlices * 2.f/100.f + -1.f + interslice*0.1), //from Wavelet's default dims (128), spacing (2/100) and origin (-1) respectively
                          (y*128.f/gridSteps * 2.f/100.f + -1.f),
                          (z*128.f/gridSteps * 2.f/100.f + -1.f));
              vec3f corner1(
                          (s*128.f/numSlices * 2.f/100.f + -1.f),
                          ((y+1)*128.f/gridSteps * 2.f/100.f + -1.f),
                          (z*128.f/gridSteps * 2.f/100.f + -1.f));
              vec3f corner2(
                          (s*128.f/numSlices * 2.f/100.f + -1.f - interslice*0.2),
                          (y*128.f/gridSteps * 2.f/100.f + -1.f),
                          ((z+1)*128.f/gridSteps * 2.f/100.f + -1.f));
              vec3f corner3(
                          (s*128.f/numSlices * 2.f/100.f + -1.f),
                          ((y+1)*128.f/gridSteps * 2.f/100.f + -1.f),
                          ((z+1)*128.f/gridSteps * 2.f/100.f + -1.f));

              size_t i0 = vertex.size();

              float wv = getWaveletValue(corner0);

              //poor man's threshold
              if (wv < thresholdLow || wv > thresholdHigh) continue;

              //poor man's ParaView lookuptable
              const float minV = -3;
              const float maxV = 3;
              if (wv < minV) wv = minV;
              if (wv > maxV) wv = maxV;
              wv = 2*(wv-minV)/(maxV-minV) - 1; //scale to -1..1
              float r = (wv>0)?1:(wv>-0.5)?(wv+0.5)*2:0;
              float g = (wv>0.5)?0:(wv>-0.5)?(0.5-abs(wv))*2:0;
              float b = (wv<0)?1:(wv<0.5)?(1-(wv+0.5))*2:0;

              vertex.push_back(corner0);
              vertex.push_back(corner1);
              vertex.push_back(corner2);
              vertex.push_back(corner3);

              color.push_back(vec4f(r,g,b, 1.0f));
              color.push_back(vec4f(r,g,b, 1.0f));
              color.push_back(vec4f(r,g,b, 1.0f));
              color.push_back(vec4f(r,g,b, 1.0f));

              index.push_back(vec3ui(i0, i0+1, i0+3));
              index.push_back(vec3ui(i0, i0+3, i0+2));

              actualTriangles = actualTriangles + 2;
          }
      }
  }
  parameters.child("actualTriangles").setValue(actualTriangles);

  // Create sg subtree
  auto &mesh = xfm.createChild("mesh", "geometry_triangles");

  mesh.createChildData("vertex.position", vertex);
  mesh.createChildData("vertex.color", color);
  mesh.createChildData("index", index);

  // Assign the scenegraph default material
  mesh.createChild("material", "uint32_t", (uint32_t)0);
  mesh.child("material").setSGOnly();
}

} // namespace sg
} // namespace ospray
