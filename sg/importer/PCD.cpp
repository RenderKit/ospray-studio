// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Importer.h"
// rkcommon
#include "rkcommon/os/FileName.h"

namespace ospray {
namespace sg {

struct PCDImporter : public Importer
{
  PCDImporter() = default;
  ~PCDImporter() override = default;

  void importScene() override;
};

OSP_REGISTER_SG_NODE_NAME(PCDImporter, importer_pcd);

// Data structures ///////////////////////////////////////////////////////
struct Field
{
  std::string name{""};
  int size{0};
  char type = (char)0;
  int count{0}; // number of channels per field
  int offset{0};
};

struct HeaderData
{
  std::vector<Field> fields;
  int width, height;
  vec3f translation;
  quaternionf quaternion;
  int numPoints;
  std::string dataType{"ascii"};
  unsigned int dataId = 0;
  int currentOffset = 0;
  int startIndex = 0;
};

struct PCDData
{
  NodePtr spheres;
  HeaderData hData;
  NodePtr baseXfm;
  std::vector<float> fileData;
};

///////////////////////////////////////////////////////////////////////////////////////////
// read ascii header

int readPCDHeader(const FileName &fileName, HeaderData &hData)
{
  std::ifstream fs;
  fs.open(fileName.c_str(), std::ios::binary);

  if (!fs.is_open() || fs.fail()) {
    throw std::runtime_error(
        "[PCDImporter::readHeader] could not open PCD file '"
        + (std::string)fileName + "'.");
    fs.close();
    return -1;
  }

  fs.seekg(0, std::ios::beg);

  try {
    while (!fs.eof()) {
      std::string line;
      std::vector<std::string> st;

      getline(fs, line);

      if (line.empty())
        continue;

      const std::string &whitespace = " \t";

      // trim any starting or trailing spaces
      const auto strBegin = line.find_first_not_of(whitespace);
      if (strBegin == std::string::npos)
        printf(
            "[PCDImporter::readHeader] error in getting line from Header \n");
      const auto strEnd = line.find_last_not_of(whitespace);
      const auto strRange = strEnd - strBegin + 1;
      line = line.substr(strBegin, strRange);

      // tokenize line
      auto delim = (char *)" ";
      char *token = std::strtok((char *)line.c_str(), delim);
      while (token) {
        st.push_back(token);
        token = std::strtok(NULL, delim);
      }

      std::stringstream sstream(line);
      sstream.imbue(std::locale::classic());

      std::string lineValue;
      sstream >> lineValue;

      if (lineValue.substr(0, 1) == "#") {
        lineValue.clear();
        continue;
      }

      if (lineValue.substr(0, 7) == "VERSION") {
        lineValue.clear();
        continue;
      }

      if (lineValue.substr(0, 6) == "FIELDS") {
        auto numEntries = st.size() - 1;
        hData.fields.resize(numEntries);

        for (int i = 0; i < numEntries; ++i) {
          hData.fields[i].name = st.at(i + 1);
          if (hData.fields[i].name == "x")
            hData.startIndex = i;
          hData.fields[i].offset = i;
        }
        continue;
      }

      if (lineValue.substr(0, 4) == "SIZE") {
        auto numEntries = st.size() - 1;
        if (numEntries != hData.fields.size())
          throw "Number of elements in Fields does not match number in Size";

        for (int i = 0; i < numEntries; ++i) {
          hData.fields[i].size = std::stoi(st.at(i + 1));
        }
        continue;
      }

      if (lineValue.substr(0, 4) == "TYPE") {
        if (hData.fields[0].size == 0)
          std::cout << "TYPE of FIELDS specified before SIZE in header!"
                    << std::endl;

        auto numEntries = st.size() - 1;
        if (numEntries != hData.fields.size())
          throw "Number of elements in Fields does not match number in Type";

        for (int i = 0; i < numEntries; ++i) {
          hData.fields[i].type = st.at(i + 1).c_str()[0];
        }

        continue;
      }

      if (lineValue.substr(0, 5) == "COUNT") {
        if (hData.fields[0].size == 0 || hData.fields[0].type == (char)0)
          throw "COUNT of FIELDS specified before SIZE or TYPE in header!";
        auto numEntries = st.size() - 1;

        if (numEntries != hData.fields.size())
          throw "Number of elements in Fields does not match number in Type";

        for (int i = 0; i < numEntries; ++i) {
          hData.fields[i].count = std::stoi(st.at(i + 1));
        }
        continue;
      }

      if (lineValue.substr(0, 5) == "WIDTH") {
        hData.width = std::stoi(st.at(1));
        if (sstream.fail())
          throw "Invalid WIDTH value specified.";
        continue;
      }

      if (lineValue.substr(0, 6) == "HEIGHT") {
        hData.height = std::stoi(st.at(1));
        if (sstream.fail())
          throw "Invalid HEIGHT value specified.";
        continue;
      }

      if (lineValue.substr(0, 9) == "VIEWPOINT") {
        if (st.size() < 7)
          throw "Not enough number of elements in VIEWPOINT! Need 7 values (tx ty tz qw qx qy qz).";
        std::vector<float> viewpoint;
        auto numEntries = st.size() - 1;

        for (int i = 0; i < numEntries; ++i) {
          viewpoint.push_back(std::stof(st.at(i + 1)));
        }
        hData.translation = vec3f(viewpoint[0], viewpoint[1], viewpoint[2]);
        hData.quaternion =
            quaternionf(viewpoint[3], viewpoint[4], viewpoint[5], viewpoint[6]);

        continue;
      }

      if (lineValue.substr(0, 6) == "POINTS") {
        hData.numPoints = std::stoi(st.at(1));
        continue;
      }

      if (lineValue.substr(0, 4) == "DATA") {
        hData.dataId = static_cast<int>(fs.tellg());
        if (st.at(1).substr(0, 17) == "binary_compressed")
          hData.dataType = "binary_compressed";
        else if (st.at(1).substr(0, 6) == "binary")
          hData.dataType = "binary";
        // hData.dataType = st.at(1);
        continue;
      }

      break;
    }

    // some preliminary checks to ensure Header has safe entries
    if (hData.numPoints == 0) {
      throw("No points to read \n");
      return (-1);
    }

    if (hData.height == 0) {
      hData.height = 1;
      throw("no HEIGHT given, setting to 1 (unorganized dataset).\n");

      if (hData.width == 0)
        hData.width = hData.numPoints;
    }

    if (static_cast<int>(hData.width * hData.height) != hData.numPoints) {
      throw("HEIGHT (%d) x WIDTH (%d) != number of points (%d)\n",
          hData.height,
          hData.width,
          hData.numPoints);
      return (-1);
    }

  } catch (const std::string &exc) {
    std::cerr << exc;
  }

  fs.close();

  return (0);
}

///////////////////////////////////////////////////////////////////////////////////////////
// read ascii data

int readPCDBodyAscii(const FileName &fileName, PCDData &pcdData)
{
  std::cout << "Reading ASCII data .." << std::endl;

  std::ifstream fs;
  fs.open(fileName.c_str());
  if (!fs.is_open() || fs.fail()) {
    throw std::runtime_error(
        "[PCDImporter::readAscii] could not open PCD file '"
        + (std::string)fileName + "'.");
    fs.close();
    return -1;
  }
  auto &headerData = pcdData.hData;

  // reposition to start of data
  fs.seekg(headerData.dataId);

  unsigned int current = 0;
  std::vector<vec3f> centers;
  std::vector<vec4f> colors;

  pcdData.spheres = createNode("spheres", "geometry_spheres");

  try {
    while (current < headerData.numPoints && !fs.eof()) {
      std::string line;
      std::vector<std::string> st;
      std::istringstream is;
      is.imbue(std::locale::classic());
      getline(fs, line);
      if (line.empty())
        continue;

      const std::string &whitespace = " \t";
      // trim any starting or trailing spaces
      const auto strBegin = line.find_first_not_of(whitespace);
      if (strBegin == std::string::npos)
        printf(
            "[PCDImporter::readAscii] error in getting line from ascii data \n");
      const auto strEnd = line.find_last_not_of(whitespace);
      const auto strRange = strEnd - strBegin + 1;
      line = line.substr(strBegin, strRange);

      // tokenize line
      auto delim = (char *)" ";
      char *token = std::strtok((char *)line.c_str(), delim);
      while (token) {
        st.push_back(token);
        token = std::strtok(NULL, delim);
      }

      if (current >= headerData.numPoints)
        printf(
            "[PCDImporter::readAscii] Reading more points than specified in the Header \n");

      std::size_t total = 0;
      std::vector<float> coordinates(3);

      for (int i = 0; i < headerData.fields.size(); ++i) {
        auto countSize = headerData.fields[i].count;
        if (countSize > 1)
          std::cout << "using only first channel for every field" << std::endl;

        auto type = headerData.fields[i].type;

        if (i < 4 && type != 'F') {
          std::cout << "Do not support integer type values" << std::endl;
          return (-1);
        } else if (i > 3) {
          std::cout
              << "Currently support interpretation of max 4 fields (first three as X Y Z and fourth as color)"
              << std::endl;
        }

        // ideally we should iterate here over every channel(represented by
        // count) of the field
        if (i == 0 || i == 1 || i == 2) {
          coordinates[i] = std::stof(st.at(
              total)); // should be total + c when using actual count for loop
        } else if (i == 3) {
          // interpret 4th channel as color values
          auto value = st.at(total);
          if (value != "nan" && value != "-nan") {
            float H = (float)std::atof(value.c_str());
            float R = std::fabs(H * 6.0f - 3.0f) - 1.0f;
            float G = 2.0f - std::fabs(H * 6.0f - 2.0f);
            float B = 2.0f - std::fabs(H * 6.0f - 4.0f);

            vec4f color{std::max(0.f, std::min(1.f, R)),
                std::max(0.f, std::min(1.f, G)),
                std::max(0.f, std::min(1.f, B)),
                0.5f};
            colors.push_back(color);
          } else
            colors.push_back(vec4f(0.5f)); // default gray color
        }

        total += headerData.fields[i].count;
      }

      vec3f origin(coordinates[0], coordinates[1], coordinates[2]);
      centers.push_back(origin);
      current++;
    }
  } catch (const std::string &exc) {
    std::cerr << exc;
  }

  pcdData.spheres->createChildData("sphere.position", centers);

  if (!colors.empty())
    pcdData.spheres->createChildData("color", colors);

  fs.close();
  return (0);
}

// following lzfDecompress() function was taken directly from the PCL library
// TODO: Add reference link to their website ?
unsigned int lzfDecompress(const void *const in_data,
    unsigned int in_len,
    void *out_data,
    unsigned int out_len)
{
  unsigned char const *ip = static_cast<const unsigned char *>(in_data);
  unsigned char *op = static_cast<unsigned char *>(out_data);
  unsigned char const *const in_end = ip + in_len;
  unsigned char *const out_end = op + out_len;

  do {
    unsigned int ctrl = *ip++;

    // Literal run
    if (ctrl < (1 << 5)) {
      ctrl++;

      if (op + ctrl > out_end) {
        std::cout << "The argument list is too long" << std::endl;
        return (0);
      }

      // Check for overflow
      if (ip + ctrl > in_end) {
        std::cout << "Invalid argument" << std::endl;
        return (0);
      }
      for (unsigned ctrl_c = ctrl; ctrl_c; --ctrl_c)
        *op++ = *ip++;
    }
    // Back reference
    else {
      unsigned int len = ctrl >> 5;

      unsigned char *ref = op - ((ctrl & 0x1f) << 8) - 1;

      // Check for overflow
      if (ip >= in_end) {
        std::cout << "Invalid argument" << std::endl;
        return (0);
      }
      if (len == 7) {
        len += *ip++;
        // Check for overflow
        if (ip >= in_end) {
          std::cout << "Invalid argument" << std::endl;
          return (0);
        }
      }
      ref -= *ip++;

      if (op + len + 2 > out_end) {
        std::cout << "The argument list is too long" << std::endl;
        return (0);
      }

      if (ref < static_cast<unsigned char *>(out_data)) {
        std::cout << "Invalid argument" << std::endl;
        return (0);
      }

      if (len > 9) {
        len += 2;

        if (op >= ref + len) {
          // Disjunct areas
          memcpy(op, ref, len);
          op += len;
        } else {
          // Overlapping, use byte by byte copying
          do
            *op++ = *ref++;
          while (--len);
        }
      } else
        for (unsigned len_c = len + 2 /* case 0 iterates twice */; len_c;
             --len_c)
          *op++ = *ref++;
    }
  } while (ip < in_end);

  return (
      static_cast<unsigned int>(op - static_cast<unsigned char *>(out_data)));
}

///////////////////////////////////////////////////////////////////////////////////////////
// read binary compressed and binary data

int readPCDBodyBinary(const FileName &fileName, PCDData &pcdData)
{
  std::cout << "Reading binary data .." << std::endl;

  // Using C++ ifstream instead of mmap, TODO: performance comparison of two
  std::ifstream file((std::string)fileName, std::ios::binary);

  if (!file.is_open() || file.fail()) {
    throw std::runtime_error(
        "[PCDImporter::readBinary] could not open PCD file '"
        + (std::string)fileName + "'.");
    file.close();
    return -1;
  }

  // calculate the file size
  file.seekg(0, std::ios::end);
  auto fileSize = file.tellg();

  int offset = 0;
  auto dataId = pcdData.hData.dataId;
  auto dataType = pcdData.hData.dataType;

  // map size calculation starting at dataId
  std::size_t mapSize = offset + dataId;

  auto numChannels = pcdData.hData.fields.size();

  if (dataType == "binary_compressed") {
    // reset to end of header
    file.seekg(offset + dataId);
    auto result = file.tellg();
    if (result < 0) {
      file.close();
      printf("[PCDImporter::readBinary] Reading compressed binary data.. \n");
      printf(
          "[PCDImporter::readBinary] Error during seeking at data offset!\n");
      return (-1);
    }

    // calculate the compressed size of the data
    unsigned int compSize = 0;
    file.read(reinterpret_cast<char *>(&compSize), 4);
    auto numRead = file.tellg();

    if (numRead < 0) {
      file.close();
      printf("[PCDImporter::readBinary] Reading compressed binary data.. \n");
      printf(
          "[PCDImporter::readBinary] Error during reading at data offset!\n");
      return (-1);
    }
    mapSize += compSize;
    mapSize += 8;

    // reset position to beginning of file
    file.seekg(0, std::ios::beg);
  } else {
    mapSize += pcdData.hData.height * pcdData.hData.width * numChannels * 4;
  }

  if (mapSize > fileSize) {
    file.close();
    printf(
        "[PCDImporter::readBinary] total file size if smaller than predicted size, please check for corruption!\n");
    return (-1);
  }

  // start reading file data into map;
  char *map;
  map = new char[mapSize];
  file.read(map, mapSize);

  file.close();

  // hard coding totalSize for data in numChannel-fields of size float
  unsigned int totalSize = pcdData.hData.numPoints * numChannels * 4;

  if (dataType == "binary_compressed") {
    // check compressed and uncompressed size
    unsigned int compSize = 0, uncompSize = 0;

    if (mapSize > dataId + 8) { // parametrization check
      memcpy(&compSize, &map[dataId + 0], 4);
      memcpy(&uncompSize, &map[dataId + 4], 4);
    }

    printf(
        "[PCDImporter::readBinary] Now reading binary compressed file with %u bytes compressed and %u original.\n",
        compSize,
        uncompSize);

    if (uncompSize != totalSize) {
      printf(
          "[PCDImporter::readBinary] The estimated total data size (%u) is different than the saved uncompressed value (%u)! Data corruption?\n",
          totalSize,
          uncompSize);
    }

    unsigned int dataSize = static_cast<unsigned int>(totalSize);
    std::vector<char> buf(dataSize);

    unsigned int tmpSize =
        lzfDecompress(&map[dataId + 8], compSize, &buf[0], dataSize);

    std::cout << "Finished decompressing binary data .. " << std::endl;

    // The size of the uncompressed data should be same as provided in the
    // header
    if (tmpSize != uncompSize) {
      printf(
          "[PCDImporter::readBinary] Size of decompressed lzf data (%u) does not match value stored in PCD header (%u).\n",
          tmpSize,
          uncompSize);
      return (-1);
    }

    // Unpack data from SOA to AOS format
    std::vector<char *> pters(numChannels);
    std::size_t toff = 0;
    for (std::size_t i = 0; i < pters.size(); ++i) {
      pters[i] = &buf[toff];
      toff += 4 * pcdData.hData.width * pcdData.hData.height;
    }

    pcdData.fileData.resize(pcdData.hData.width * pcdData.hData.height * 4);

    for (auto i = 0; i < pcdData.hData.width * pcdData.hData.height; ++i) {
      // int k = 0;
      for (std::size_t j = 0; j < pters.size(); ++j) {
        auto index = i * 4 + pcdData.hData.fields[j].offset;
        if (pters[j] && index < pcdData.fileData.size())
          memcpy(&pcdData.fileData[index], pters[j], 4);
        // Increment the pointer
        pters[j] += 4;
        // k++;
      }
    }
  } else {
    pcdData.fileData.resize(
        pcdData.hData.width * pcdData.hData.height * numChannels);
    if (totalSize <= mapSize)
      memcpy(&pcdData.fileData[0], &map[0] + dataId, totalSize);
  }

  delete[] map;

  std::vector<vec3f> centers;
  std::vector<vec4f> colors;

  pcdData.spheres = createNode("spheres", "geometry_spheres");

  auto startIndex = pcdData.hData.startIndex;

  for (auto i = 0; i < pcdData.fileData.size(); i += numChannels) {
    vec3f center{pcdData.fileData[i + startIndex],
        pcdData.fileData[i + startIndex + 1],
        pcdData.fileData[i + startIndex + 2]};
    centers.push_back(center);

    // has color and additional channels, however only one channel interpreted
    // as color atm
    if (numChannels > 3) {
      auto value = pcdData.fileData[i + startIndex + 3];
      if (!isnan(value)) {
        float H = value;
        float R = std::fabs(H * 6.0f - 3.0f) - 1.0f;
        float G = 2.0f - std::fabs(H * 6.0f - 2.0f);
        float B = 2.0f - std::fabs(H * 6.0f - 4.0f);

        vec4f color{std::max(0.f, std::min(1.f, R)),
            std::max(0.f, std::min(1.f, G)),
            std::max(0.f, std::min(1.f, B)),
            0.5f};
        colors.push_back(color);
      } else
        colors.push_back(vec4f(0.f, 0.5f, 0.5f, 1.f));
    } else
      colors.push_back(vec4f(0.f, 0.5f, 0.5f, 1.f));
  }

  pcdData.spheres->createChildData("sphere.position", centers);
  pcdData.spheres->createChildData("color", colors);

  std::cout << "Number of rendered points : " << centers.size() << std::endl;

  return (0);
}

int readPCD(const FileName &fileName, PCDData &pcdData)
{
  auto &headerData = pcdData.hData;
  // read header
  auto result = readPCDHeader(fileName, headerData);

  if (result < 0)
    return (result);

  if (headerData.dataType == "ascii")
    result = readPCDBodyAscii(fileName, pcdData);
  else
    result = readPCDBodyBinary(fileName, pcdData);

  return result;
}

// PCDImporter definitions /////////////////////////////////////////////

void PCDImporter::importScene()
{
  PCDData pcdData;
  auto res = readPCD(fileName, pcdData);
  if (res < 0)
    std::cout << "Could not read PCD file" << std::endl;

  // Create a root Transform/Instance off the Importer, under which to build
  // the import hierarchy
  std::string baseName = fileName.name() + "_rootXfm";
  auto rootNode = createNode(baseName, "transform");

  // use VIEWPOINT value from the Header to set base transform value for the pcd
  // data
  rootNode->child("translation") = pcdData.hData.translation;
  rootNode->child("rotation") = pcdData.hData.quaternion;

  materialRegistry->add(createNode("default-material-pcd", "obj"));
  const std::vector<uint32_t> mID = {0};
  pcdData.spheres->createChildData(
      "material", mID); // This is a scenegraph parameter
  pcdData.spheres->child("material").setSGOnly();
  pcdData.spheres->child("radius").setValue(pointSize);

  if (pcdData.spheres->hasChild("color"))
    pcdData.spheres->child("color").setSGOnly();

  rootNode->add(pcdData.spheres);

  add(rootNode);
  std::cout << "Finished importing PCD file." << std::endl;
}

} // namespace sg
} // namespace ospray
