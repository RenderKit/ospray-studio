#pragma once

#include <vector>

namespace ospray {
namespace storyboard_plugin {

class ScreenShotContainer
{
public:
    ScreenShotContainer();
    ~ScreenShotContainer();
    
    void setImage(int w, int h, std::vector<unsigned char> values);
    void clear();

    bool isNextChunkAvailable();
    std::vector<unsigned char> getNextChunk(int len);
    int getStartIndex();

private:
    std::vector<unsigned char> imgValues;
    int width;
    int height;
    int currStartIndex;
};

}  // namespace storyboard_plugin
}  // namespace ospray