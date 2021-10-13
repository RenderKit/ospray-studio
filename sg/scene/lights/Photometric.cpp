// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Photometric.h"

// This file parses EULUMDAT photometric light parameters as specified here:
// https://evo.support-en.dial.de/support/solutions/articles/9000074164-eulumdat-format

// Spotlights may have a measured intensityDistribution as per the OSPRay spec.
// https://www.ospray.org/documentation.html#spotlight-photometric-light

namespace ospray {
namespace sg {

// Value parse helpers

//#define PRINT_VALUES

#if defined(PRINT_VALUES)
#define PRINTVAL(X) std::cout << "   " << #X << ": " << X << std::endl;
#else
#define PRINTVAL(X)
#endif

template <>
std::string Eulumdat::getValueAs(int max)
{
  char temp[256] = {};
  try {
    openFile.getline(temp, 256);
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    throw std::runtime_error("#studio:sg: Eulumdat parse error");
  }
  return std::string(temp, 0, max);
}
template <>
int Eulumdat::getValueAs(int max)
{
  int value = 0;
  try {
    value = stoi(getValueAs<std::string>(max));
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    throw std::runtime_error("#studio:sg: Eulumdat parse error");
  }
  return value;
}
template <>
float Eulumdat::getValueAs(int max)
{
  float value = 0.f;
  try {
    value = stof(getValueAs<std::string>(max + 1)); // add decimal point
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    throw std::runtime_error("#studio:sg: Eulumdat parse error");
  }
  return value;
}

// File loader

bool Eulumdat::load()
{
  if (fileName == "") {
    std::cerr << "#studio:sg: no Eulumdat file set" << std::endl;
    return false;
  }

  openFile = std::ifstream(fileName);
  if (!openFile.good()) {
    std::cerr << "#studio:sg: cannot open Eulumdat file " << fileName
              << std::endl;
    return false;
  }

  try {
    // In line order of the EULUMDAT file
    // [...] is number of characters/digits in field

    // Manufacturer name/database/version/format [max 78]
    ID = getValueAs<std::string>(78);
    PRINTVAL(ID);

    // Type indicator Itype [1]
    iType = getValueAs<int>(1);
    PRINTVAL(iType);

    // Symmetry indicator Isym [1]
    iSym = getValueAs<int>(1);
    PRINTVAL(iSym);

    // Number Mc of C-planes for 0 ... 360° [2]
    Mc = getValueAs<int>(2);
    PRINTVAL(Mc);

    // Distance Dc between C-planes [5]
    Dc = getValueAs<float>(5);
    PRINTVAL(Dc);

    // Number of Ng of the luminous intensities in every C-level [2]
    Ng = getValueAs<int>(2);
    PRINTVAL(Ng);

    // Distance Dg between the luminous intensities [5]
    Dg = getValueAs<float>(5);
    PRINTVAL(Dg);

    // Number of test certificate [max 78]
    numCert = getValueAs<std::string>(78);
    PRINTVAL(numCert);

    // Luminaire name [max 78]
    lumName = getValueAs<std::string>(78);
    PRINTVAL(lumName);

    // Luminaire number [max 78]
    lumNum = getValueAs<std::string>(78);
    PRINTVAL(lumNum);

    // File name [8]
    file = getValueAs<std::string>(8 + 4); // 8 + .ext
    PRINTVAL(file);

    // Date/person responsible [max 78]
    date = getValueAs<std::string>(78);
    PRINTVAL(date);

    // Length/diameter of the luminaire (mm) [4]
    lampLength = getValueAs<float>(4);
    PRINTVAL(lampLength);

    // Width of the luminaire (0 for circular luminaires) (mm) [4]
    B0 = getValueAs<float>(4);
    PRINTVAL(B0);

    // Height of the luminaire (mm) [4]
    height = getValueAs<float>(4);
    PRINTVAL(height);

    // Length/diameter of the luminous area (mm) [4]
    luminousLength = getValueAs<float>(4);
    PRINTVAL(luminousLength);

    // Width of the luminous area (0 for a circular area) (mm) [4]
    B1 = getValueAs<float>(4);
    PRINTVAL(B1);

    // Height of the luminous areas C0, C90, C180 & C270 (mm) [4]
    heightC0 = getValueAs<float>(4);
    heightC90 = getValueAs<float>(4);
    heightC180 = getValueAs<float>(4);
    heightC270 = getValueAs<float>(4);
    PRINTVAL(heightC0);
    PRINTVAL(heightC90);
    PRINTVAL(heightC180);
    PRINTVAL(heightC270);

    // Ratio of the downward flux fraction (%) [4]
    DFF = getValueAs<float>(4);
    PRINTVAL(DFF);

    // Light output ratio of the luminaire EtaLB (%) [4]
    LORL = getValueAs<float>(4);
    PRINTVAL(LORL);

    // Conversation factor for luminous intensity [6]
    CFLI = getValueAs<float>(4);
    PRINTVAL(CFLI);

    // Tilt of the luminaire during measurement [6]
    // (important for street luminaires)
    tilt = getValueAs<float>(4);
    PRINTVAL(tilt);

    // Number of equipment (optional) and per equipment info [4]
    numEquipment = getValueAs<int>(4);
    PRINTVAL(numEquipment);
    if (numEquipment > 0) {
      for (auto i = 1; i <= numEquipment; i++) {
        EquipmentInfo thisLamp;
        thisLamp.numLamps = getValueAs<int>(4); // Number of lamps [4]
        thisLamp.type = getValueAs<std::string>(24); // Type of lamp [24]
        thisLamp.flux = getValueAs<float>(12); // Total luminous flux (lm) [12]
        thisLamp.K = getValueAs<std::string>(16); // Colour temperature [16]
        thisLamp.CRI = getValueAs<std::string>(6); // Colour rendering index [6]
        thisLamp.power = getValueAs<float>(8); // Total system power [8]
        equipmentInfo.push_back(thisLamp);

        PRINTVAL(i);
        PRINTVAL(thisLamp.numLamps);
        PRINTVAL(thisLamp.type);
        PRINTVAL(thisLamp.flux);
        PRINTVAL(thisLamp.K);
        PRINTVAL(thisLamp.CRI);
        PRINTVAL(thisLamp.power);
      }
    }

    // Ratio of total luminous flux half-space for k = 0.6 ... 5 [10 x 7 ea]
    // (for determination of number of lamps to light output ratio
    // procedure LITG 5th edition 1988)
    for (auto i = 0; i < 10; i++) {
      DR[i] = getValueAs<float>(7);
      PRINTVAL(DR[i]);
    }

    // Angle C (starting with 0°) [Mc x 6 ea]
    for (auto c = 0; c < Mc; c++) {
      float tempC = getValueAs<float>(6);
      PRINTVAL(tempC);
      C.push_back(tempC);
    }

    // Angle G (starting with 0 degrees) [Ng x 6 ea]
    for (auto g = 0; g < Ng; g++) {
      float tempG = getValueAs<float>(6);
      PRINTVAL(tempG);
      G.push_back(tempG);
    }

    // Distribution of luminous intensity (cd/klm) [(Mc2-Mc1+1) x Ng x 6 ea]
    //    for Isym = 0, Mc1 = 1 and Mc2 = Mc
    //    for Isym = 1, Mc1 = 1 and Mc2 = 1
    //    for Isym = 2, Mc1 = 1 and Mc2 = Mc/2+1
    //    for Isym = 3, Mc1 = 3*Mc/4+1 and Mc2 = Mc1+Mc/2
    //    for Isym = 4, Mc1 = 1 and Mc2 = Mc/4+1
    auto makeMc = [&]() {
      switch (iSym) {
      case 0:
        return Mc;
      case 1:
        return 1;
      case 2:
        return Mc / 2 + 1;
      case 3:
        return Mc / 2 + 1;
      case 4:
        return Mc / 4 + 1;
      default:
        return -1;
      }
    };
    totalMc = makeMc();
    if (totalMc < 0)
      throw std::runtime_error("Eulumdat totalMc calculation is wrong!");

    lid.resize(totalMc * Ng);
    for (auto c = 0; c < totalMc; c++)
      for (auto g = 0; g < Ng; g++)
        lid[c * Ng + g] = getValueAs<float>(6);

#if defined(PRINT_VALUES) // Print lid table
    for (auto c = 0; c < totalMc; c++)
      std::cout << "\t" << C[c];
    std::cout << std::endl;
    for (auto g = 0; g < Ng; g++) {
      std::cout << G[g] << ":";
      for (auto c = 0; c < totalMc; c++)
        std::cout << "\t" << (lid.data()[c * Ng + g]);
      std::cout << std::endl;
    }
#endif

  } catch (const std::exception &e) {
    std::cerr << "#studio:sg: Eulumdat parse error" << std::endl;
    std::cerr << e.what() << std::endl;
    // Clear any vectors as values may be wrong.
    equipmentInfo.clear();
    C.clear();
    G.clear();
    lid.clear();
    return false;
  }
  return true;
}

} // namespace sg
} // namespace ospray
