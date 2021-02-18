/**
 * @copyright This program is the confidential and proprietary product of Valeo Driving Assistance Systems
 *            Research (DAR). Any unauthorised use, reproduction or transfer of this program is strictly prohibited
 *            (subject to limited distribution and restricted disclosure only). All rights reserved.
 *
 * @author Marco Heinen <marco.heinen@valeo.com>, Anja Kleinke <anja.kleinke@valeo.com>
 *
 * @date February 2021
 *
 * @file
 */

#ifndef VALEOLIDAR_H
#define VALEOLIDAR_H

// DO NOT CHANGE

#include <cmath>
#include <array>
#include <string>
#include <memory>


// DO NOT CHANGE ANY OF THE FOLLOWING:


#define PI (4.0 * std::atan(1.0))
#define DEG2RAD (PI / 180.0)
#define RAD2DEG (1.0 / DEG2RAD)

#define APD_HORIZ_APERTURE_DEG 0.125

#define MAX_NUM_APDS 200
#define MAX_NUM_COLUMNS 2880

#define APD_AV_WIDTH_PIXEL 6
#define APD_AV_HEIGHT_PIXEL 32

#define MAX_PIXELS_HOR (MAX_NUM_COLUMNS * APD_AV_WIDTH_PIXEL)
#define MAX_PIXELS_VER (MAX_NUM_APDS * APD_AV_HEIGHT_PIXEL)


#define APD_VERTI_PICKUP_RANGE_DEG    0.517487086
#define APD_VERTI_SEPARATION_DEG      0.085012914
#define APDGROUP_VERTI_SEPARATION_DEG 0.18422412








extern "C"
{

void processFrame(double ** ptrDistances, double ** ptrIntensities, const uint numColsP, const uint numAPDsPerGroupP, const uint numAPDGroupsP, const bool beVerboseP, const char * pcdOutputFileNameP);


}



#endif // VALEOLIDAR_H
