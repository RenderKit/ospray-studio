#include "ScreenShotContainer.h"

#include <iostream>

namespace ospray {
namespace storyboard_plugin {

ScreenShotContainer::ScreenShotContainer() { }

ScreenShotContainer::~ScreenShotContainer() { }

void ScreenShotContainer::setImage(int w, int h, std::vector<unsigned char> values) {
    imgValues.insert(imgValues.end(), values.begin(), values.end());
    width = w;
    height = h;
    currStartIndex = 0;
}

void ScreenShotContainer::clear() {
    imgValues.clear();
    width = 0;
    height = 0;
    currStartIndex = 0;
}

bool ScreenShotContainer::isNextChunkAvailable() {
    return currStartIndex < imgValues.size();
}

std::vector<unsigned char> ScreenShotContainer::getNextChunk(int len) {
    int chunkLen = (currStartIndex + len) > imgValues.size() ? (imgValues.size() - currStartIndex) : len;

    // Starting and Ending iterators
    auto start = imgValues.begin() + currStartIndex;
    auto end = imgValues.begin() + currStartIndex + chunkLen;

    // To store the sliced vector
    std::vector<unsigned char> result(chunkLen);
    copy(start, end, result.begin());
    currStartIndex += chunkLen;
 
    // Return the final sliced vector
    return result;
}

int ScreenShotContainer::getStartIndex() {
    return currStartIndex;
}

}  // namespace storyboard_plugin
}  // namespace ospray