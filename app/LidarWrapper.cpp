// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "LidarWrapper.h"
#include "valeo_lidar.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>



//#define DEBUG_OUTPUT

float writeDebugPPM(const float rgba[LIDAR_FRAMEBUFFER_HEIGHT][LIDAR_FRAMEBUFFER_WIDTH][4]){

  const std::string filename = "DebugPPM.ppm";

  const unsigned int horPixels = LIDAR_FRAMEBUFFER_WIDTH;
  const unsigned int vertPixels = LIDAR_FRAMEBUFFER_HEIGHT;

  std::ofstream outputfile(filename, std::ios::out);
  if (!outputfile.good() || !outputfile.is_open()) {
    std::stringstream errMsg;
    errMsg << "Could not open file '" << filename << "' for writing.";
    throw std::runtime_error(errMsg.str());
  }
  float maxValue = 0;
  for(uint j=0; j < vertPixels; j++){
      for(uint i = 0; i < horPixels; i++){
          if(rgba[j][i][0]>maxValue) maxValue = rgba[j][i][0];
          if(rgba[j][i][1]>maxValue) maxValue = rgba[j][i][1];
          if(rgba[j][i][2]>maxValue) maxValue = rgba[j][i][2];
      }
  }
  std::cout<<"max value "<<maxValue<<std::endl;

  outputfile << "P6\n"
          << horPixels<<" "<<vertPixels<<"\n"
          << "255\n";

  for(uint j=0; j < vertPixels; j++){
    for(uint i = 0; i < horPixels; i++)
    {
      uint l = vertPixels - j -1; //gl image is upside down

      unsigned char r = static_cast<unsigned char>(rgba[l][i][0] * 255./maxValue);
      unsigned char g = static_cast <unsigned char>(rgba[l][i][1] * 255./maxValue);
      unsigned char b = static_cast <unsigned char>(rgba[l][i][2] * 255./maxValue);


      unsigned char out[3] {r,g,b};
      outputfile.write(reinterpret_cast<char*>(out),3);

    }
  }

  outputfile.close();
  return maxValue;
}


void writeDebugPCDBinary(const double distArray[LIDAR_FRAMEBUFFER_HEIGHT][LIDAR_FRAMEBUFFER_WIDTH],
                         const double intensityArray[LIDAR_FRAMEBUFFER_HEIGHT][LIDAR_FRAMEBUFFER_WIDTH]){
    const std::string& filename = "DebugPointcloud.pcd";
    int numberOfPointHor = LIDAR_FRAMEBUFFER_WIDTH;
    int numberOfPointsVer = LIDAR_FRAMEBUFFER_HEIGHT;
    std::ofstream outputfile(filename, std::ios::out);
    if (!outputfile.good() || !outputfile.is_open()) {
      std::stringstream errMsg;
      errMsg << "Could not open file '" << filename << "' for writing.";
      throw std::runtime_error(errMsg.str());
    }
    outputfile << "# .PCD v.7 - Point Cloud Data file format\n"
               << "VERSION .7\n"
               << "FIELDS nuid x y z intensity\n"
               << "SIZE 4 4 4 4 4\n"
               << "TYPE F F F F F\n"
               << "COUNT 1 1 1 1 1\n"
               << "WIDTH " << numberOfPointHor * numberOfPointsVer << "\n"
               << "HEIGHT 1\n"
               << "VIEWPOINT 0 0 0 1 0 0 0\n"
               << "POINTS " << numberOfPointHor * numberOfPointsVer << "\n"
               << "DATA binary\n";
    for (uint cnt_SegVer = 0; cnt_SegVer <  LIDAR_FRAMEBUFFER_HEIGHT; cnt_SegVer++) {
      for (uint cnt_SegHor = 0; cnt_SegHor < LIDAR_FRAMEBUFFER_WIDTH; cnt_SegHor++) {
        double angle_h = -HORIZ_FOV_DEG/2.0;
        angle_h += HORIZ_FOV_DEG / static_cast<double>(LIDAR_FRAMEBUFFER_WIDTH) / 2.0;
        angle_h += HORIZ_FOV_DEG * static_cast<double>(cnt_SegHor) / static_cast<double>(LIDAR_FRAMEBUFFER_WIDTH);
        double angle_v = -VERTI_FOV_DEG/ 2.0;
        angle_v += VERTI_FOV_DEG / static_cast<double>(LIDAR_FRAMEBUFFER_HEIGHT) / 2.0;
        angle_v += VERTI_FOV_DEG * cnt_SegVer / static_cast<double>(LIDAR_FRAMEBUFFER_HEIGHT);

        double AngleHor = DEG2RAD * angle_h;
        double AngleVer = DEG2RAD * angle_v;
        double distance = distArray[cnt_SegVer][cnt_SegHor];
        double intensity = intensityArray[cnt_SegVer][cnt_SegHor];


        float n,x,y,z,r;
        if (std::isfinite(distance)){
           n = static_cast<float>(cnt_SegVer * LIDAR_FRAMEBUFFER_WIDTH + cnt_SegHor);
           x = static_cast<float>(distance * cos(AngleVer) * sin(AngleHor));
           y = static_cast<float>(distance * sin(AngleVer));
           z = static_cast<float>(distance * cos(AngleVer) * cos(AngleHor));
           r = static_cast<float>(intensity);
        }
        else {
            n = static_cast<float>(cnt_SegVer * LIDAR_FRAMEBUFFER_WIDTH + cnt_SegHor);
            x = 0;
            y = 0;
            z = 0;
            r = 0;
        }
        float out[5] {n,x,y,z,r};
        outputfile.write(reinterpret_cast<char*>(out), 20);
        }
      }
    outputfile.close();
}


void LidarProcessFrame(
    const float depths[LIDAR_FRAMEBUFFER_HEIGHT][LIDAR_FRAMEBUFFER_WIDTH],
    const float rgba[LIDAR_FRAMEBUFFER_HEIGHT][LIDAR_FRAMEBUFFER_WIDTH][4],
    const int verbose,
    const char *file_name)
{



#ifdef DEBUG_OUTPUT
  std::cout<<"Compare FOV parameters with correct parameters for 1440 columns, 4 APDs per Group and 20 APD groups"<<std::endl;
  std::cout<<"Framebuffer Width: "<<LIDAR_FRAMEBUFFER_WIDTH<<", correct: 8640"<<std::endl;
  std::cout<<"Framebuffer Height: "<<LIDAR_FRAMEBUFFER_HEIGHT<<", correct 2560"<<std::endl;
  std::cout<<"Image Start Hor: "<<LIDAR_IMAGE_START_HOR<<", correct 0.25"<<std::endl;
  std::cout<<"Image End Hor: "<<LIDAR_IMAGE_END_HOR<<", correct 0.75"<<std::endl;
  std::cout<<"Image Start Ver: "<<LIDAR_IMAGE_START_VER<<", correct 0.361"<<std::endl;
  std::cout<<"Image End ver: "<<LIDAR_IMAGE_END_VER<<", correct 0.638"<<std::endl;
#endif
#ifdef DEBUG_OUTPUT
  float maxValue = writeDebugPPM(rgba);
#endif

  auto distance = new double[LIDAR_FRAMEBUFFER_HEIGHT][LIDAR_FRAMEBUFFER_WIDTH];
  for (int y = 0; y < LIDAR_FRAMEBUFFER_HEIGHT; y++) {
    for (int x = 0; x < LIDAR_FRAMEBUFFER_WIDTH; x++)
      distance[LIDAR_FRAMEBUFFER_HEIGHT - 1 - y][x] = depths[y][x];
  }

  auto intensity = new double[LIDAR_FRAMEBUFFER_HEIGHT][LIDAR_FRAMEBUFFER_WIDTH];
  for (int y = 0; y < LIDAR_FRAMEBUFFER_HEIGHT; y++) {
    for (int x = 0; x < LIDAR_FRAMEBUFFER_WIDTH; x++)
      intensity[LIDAR_FRAMEBUFFER_HEIGHT - 1 - y][x] =
          rgba[y][x][0]+ rgba[y][x][1] * 0.1f + rgba[y][x][2] * 0.01f;
  }

  auto instID = new unsigned int[LIDAR_FRAMEBUFFER_HEIGHT][LIDAR_FRAMEBUFFER_WIDTH];
  for (int y = 0; y < LIDAR_FRAMEBUFFER_HEIGHT; y++) {
    for (int x = 0; x < LIDAR_FRAMEBUFFER_WIDTH; x++)
      instID[LIDAR_FRAMEBUFFER_HEIGHT - 1 - y][x] = static_cast<unsigned int>(depths[y][x] * 5);
  }



#ifdef DEBUG_OUTPUT
  writeDebugPCDBinary(distance, intensity);
#endif

  processFrame(reinterpret_cast<double *>(distance),
      reinterpret_cast<double *>(intensity),
      reinterpret_cast<unsigned int *>(instID),
      NUM_COLUMNS,
      NUM_APDS_PER_GROUP,
      NUM_APD_GROUPS,
      verbose,
      file_name);

  delete[] distance;
  delete[] intensity;
};
