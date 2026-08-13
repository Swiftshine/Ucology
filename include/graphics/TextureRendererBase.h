#pragma once

#include <math/seadMatrix.h>

class TextureRendererBase {
public:
    TextureRendererBase();

protected:
    sead::Matrix34f mMatrix;
};