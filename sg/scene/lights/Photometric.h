// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sg/Node.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE Eulumdat
{
  Eulumdat(){};
  Eulumdat(std::string _fileName) : fileName(_fileName){};
  ~Eulumdat(){};

  bool load();

  // In line order of the EULUMDAT file
  // [...] is number of characters/digits in field

  // Manufacturer name/database/version/format [max 78]
  std::string ID;

  // Type indicator Itype [1]
  int iType;

  // Symmetry indicator Isym [1]
  int iSym;

  // Number Mc of C-planes for 0 ... 360° [2]
  int Mc;

  // Distance Dc between C-planes [5]
  float Dc;

  // Number of Ng of the luminous intensities in every C-level [2]
  int Ng;

  // Distance Dg between the luminous intensities [5]
  float Dg;

  // Number of test certificate [max 78]
  std::string numCert;

  // Luminaire name [max 78]
  std::string lumName;

  // Luminaire number [max 78]
  std::string lumNum;

  // File name [8]
  std::string file;

  // Date/person responsible [max 78]
  std::string date;

  // Length/diameter of the luminaire (mm) [4]
  float lampLength;

  // Width of the luminaire (0 for circular luminaires) (mm) [4]
  float B0;

  // Height of the luminaire (mm) [4]
  float height;

  // Length/diameter of the luminous area (mm) [4]
  float luminousLength;

  // Width of the luminous area (0 for a circular area) (mm) [4]
  float B1;

  // Heights of the luminous area @ C0, C90, C180, C270 (mm) [4]
  float heightC0;
  float heightC90;
  float heightC180;
  float heightC270;

  // Ratio of the downward flux fraction (%) [4]
  float DFF;

  // Light output ratio of the luminaire EtaLB (%) [4]
  float LORL;

  // Conversation factor for luminous intensity [6]
  float CFLI;

  // Tilt of the luminaire during measurement [6]
  // (important for street luminaires)
  float tilt;

  // Number of equipment (optional) and per equipment info [4]
  int numEquipment;
  typedef struct
  {
    int numLamps; // Number of lamps [4]
    std::string type; // Type of the lamp [24]
    float flux; // Total luminous flux of lamps (lm) [12]
    std::string K; // Colour temperature of the lamp [16]
    std::string CRI; // Colour rendering index [6]
    float power; // Total system power [8]
  } EquipmentInfo;
  std::vector<EquipmentInfo> equipmentInfo;

  // Direct Ratio of total luminous flux [10 x 7 ea]
  // half-space for k = 0.6 ... 5
  // (for determination of number of lamps to light output ratio procedure
  // LITG 5th edition 1988)
  float DR[10];

  // Angle C (starting with 0°) [Mc x 6 ea]
  std::vector<float> C;

  // Angle G (starting with 0°) [Ng x 6 ea]
  std::vector<float> G;

  // Distribution of luminous intensity (cd/klm) [(Mc2-Mc1+1) x Ng x 6 ea]
  //    for Isym = 0, Mc1 = 1 and Mc2 = Mc
  //    for Isym = 1, Mc1 = 1 and Mc2 = 1
  //    for Isym = 2, Mc1 = 1 and Mc2 = Mc/2+1
  //    for Isym = 3, Mc1 = 3*Mc/4+1 and Mc2 = Mc1+Mc/2
  //    for Isym = 4, Mc1 = 1 and Mc2 = Mc/4+1
  int totalMc;
  std::vector<float> lid;

  std::string fileName{0};
  std::ifstream openFile;

  template <typename T>
  T getValueAs(int max);
};

} // namespace sg
} // namespace ospray

