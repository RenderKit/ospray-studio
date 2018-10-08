// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include <string>

namespace ospray {

  struct Panel
  {
    Panel() = default;
    virtual ~Panel() = default;

    // build the UI itself (per-frame ImGui calls)
    virtual void buildUI() = 0;

    void setShown(bool shouldBeShown);
    void toggleShown();
    bool isShown() const;

    void setName(const std::string &newName);

    std::string name() const;

  protected:

    Panel(const std::string &_name) : currentName(_name) {}

    // Properties //

    bool show = false;
    std::string currentName {"<unnamed panel>"};
  };

  // Inlined members //////////////////////////////////////////////////////////

  inline void Panel::setShown(bool shouldBeShown)
  {
    show = shouldBeShown;
  }

  inline void Panel::toggleShown()
  {
    show = !show;
  }

  inline bool Panel::isShown() const
  {
    return show;
  }

  inline void Panel::setName(const std::string &newName)
  {
    currentName = newName;
  }

  inline std::string Panel::name() const
  {
    return currentName;
  }

} // namespace ospray
