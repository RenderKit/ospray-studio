// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef USE_OPENIMAGEIO
#include <OpenImageIO/imageio.h>
#endif

// sg
#include "../Data.h"
#include "Texture.h"
// rkcommon
#include "rkcommon/os/FileName.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE Texture2D : public Texture
{
  Texture2D();
  ~Texture2D() override;

  virtual void preCommit() override;
  virtual void postCommit() override;

  //! \brief load texture from given file or memory address.
  /*! \detailed if file does not exist, or cannot be loaded for
      some reason, return NULL. Multiple loads from the same file
      will return the *same* texture object */
  void load(const FileName &fileName,
      const bool preferLinear = false,
      const bool nearestFilter = false,
      const int colorChannel = 4);  // default to sampling all channels
  void load(void *memory,
      const bool preferLinear = false,
      const bool nearestFilter = false,
      const int colorChannel = 4);  // default to sampling all channels

  std::string fileName;

  vec2ul size{-1}; //! texture size, in pixels
  int components{0};
  int depth{0};  // bytes per texel
  bool preferLinear{false};
  bool nearestFilter{false};
  int colorChannel{4};   // sampled channel R(0), G(1), B(2), A(3), all(4)
  bool flip{true}; // flip texture data vertically

  // Create the childData node given all other texture params
  inline void createDataNode()
  {
    if (depth == 1)
      createDataNodeType_internal<uint8_t>();
    else if (depth == 2)
      createDataNodeType_internal<uint16_t>();
    else if (depth == 4)
      createDataNodeType_internal<float>();
    else
      std::cerr << "#osp:sg: INVALID Texture depth " << depth << std::endl;
  }

 private:
  void *texelData{nullptr}; // XXX make this shared, avoid the copy data
  static std::map<std::string, std::shared_ptr<Texture2D>> textureCache;

#ifdef USE_OPENIMAGEIO
  void loadTexture_OIIO(const std::string &fileName);
  template <typename T>
  void loadTexture_OIIO_readFile(std::unique_ptr<OIIO::ImageInput> &in);
#else
  void loadTexture_PFM(const std::string &fileName);
  void loadTexture_STBi(const std::string &fileName);
  void loadTexture_PFM_readFile(FILE *file, float scaleFactor);
#endif

  // Internal helpers
  template <typename T>
  void createDataNodeType_internal();
  template <typename T, int N>
  void createDataNodeVec_internal();
  template <typename T>
  void createDataNodeVec_internal();

  inline OSPTextureFormat osprayTextureFormat();
};


// Check texture filename for udim pattern.  Then check that each tile file
// exists.  Image size/format will be checked on load.  This is a quick check.
inline bool checkUDIM(FileName filename)
{
  bool isUDIM = false;
  std::string fullName = filename.str();

#if 0 // XXX
  std::cout << "isUDIM... always true " << std::endl;
  return true;
#endif

  std::cout << "isUDIM... checking " << fullName << std::endl;

  // See if base tile "1001" is in the filename, if not it's not likely a UDIM
  // XXX Apparently tiles can be skipped.  Can 1001 be skipped????
  auto found = fullName.rfind("1001");
  if (found == std::string::npos)
    return false;

#if 1 // extract into a find_udim_files() return vector of valid files
  // Strip off the "1001" and continue searching for other tiles
  // by checking existing files of the correct names.
  // This will work for most any *1001* naming scheme.
  std::string lFileName = fullName.substr(0, found);
  std::string rFileName = fullName.substr(found + 4);

  std::cout << "isUDIM... pattern " << lFileName << "*" << rFileName
            << std::endl;

  int vdim = 0;
  int udim = 0;
  for (int v = 0; v < 10 && udim == 0; v++)
    for (int u = 0; u < 10; u++) {
      std::string tileNum = std::to_string(1001 + v * 10 + u);
      std::string checkName = lFileName + tileNum + rFileName;
      std::cout << "isUDIM... " << checkName << std::endl;
      std::ifstream f(checkName.c_str());
      // Exit loop on first missing tile-filename.
      if (!f.good()) {
        std::cout << "isUDIM... missing " << checkName << std::endl;
        vdim = v;
        udim = v == 0 ? u : 10;
        break;
      }
      std::cout << "isUDIM... found " << checkName << std::endl;
    }
#endif

  if (vdim > 0 || udim > 1)
    isUDIM = true;

  std::cout << "isUDIM... (" << isUDIM << ") " << vdim << ":" << udim
            << std::endl;

  return isUDIM;
}

} // namespace sg
} // namespace ospray
