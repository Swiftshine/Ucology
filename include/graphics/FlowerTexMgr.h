#pragma once

#include <graphics/TexQuadGrass.h>

// todo: move this to game headers

class FlowerTexMgr {
public:
    // Address: 0x0268A038
    void initialize();

    TexQuadGrass& getFlowerRenderer() { return mFlowerRenderer; }
    const TexQuadGrass& getFlowerRenderer() const { return mFlowerRenderer; }

    TexQuadGrass& getFlowerStalkRenderer() { return mFlowerStalkRenderer; }
    const TexQuadGrass& getFlowerStalkRenderer() const { return mFlowerStalkRenderer; }

    TexQuadGrass& getGrassRenderer() { return mGrassRenderer; }
    const TexQuadGrass& getGrassRenderer() const { return mGrassRenderer; }

    TexQuadGrass& getButterflyRenderer() { return mButterflyRenderer; }
    const TexQuadGrass& getButterflyRenderer() const { return mButterflyRenderer; }

    agl::TextureData& getFlowerTexture(u32 index) { return mFlowerTextures[index]; }
    const agl::TextureData& getFlowerTexture(u32 index) const { return mFlowerTextures[index]; }

    agl::TextureData& getFlowerStalkTexture() { return mFlowerStalkTexture; }
    const agl::TextureData& getFlowerStalkTexture() const { return mFlowerStalkTexture; }

    agl::TextureData& getButterflyTexture() { return mButterflyTexture; }
    const agl::TextureData& getButterflyTexture() const { return mButterflyTexture; }

    agl::TextureData& getFlowerTextureNormal() { return mFlowerTextureNormal; }
    const agl::TextureData& getFlowerNormalTexture() const { return mFlowerTextureNormal; }

    agl::TextureData& getFlowerStalkTextureNormal() { return mFlowerStalkTextureNormal; }
    const agl::TextureData& getFlowerStalkNormalTexture() const { return mFlowerStalkTextureNormal; }

    agl::TextureData& getGrassTextureNormal(u32 index) { return mGrassTextureNormals[index]; }
    const agl::TextureData& getGrassTextureNormal(u32 index) const { return mGrassTextureNormals[index]; }

    agl::TextureData& getGrassTexture(u32 index) { return mGrassTextures[index]; }
    const agl::TextureData& getGrassTexture(u32 index) const { return mGrassTextures[index]; }

protected:
    TexQuadGrass mFlowerRenderer;
    TexQuadGrass mFlowerStalkRenderer;
    TexQuadGrass mGrassRenderer;
    TexQuadGrass mButterflyRenderer;
    nw::g3d::res::ResFile* mResFile;
    agl::TextureData mFlowerTextures[5];    // The values of the first two are copied into the last two
    agl::TextureData mFlowerStalkTexture;
    agl::TextureData mButterflyTexture;
    agl::TextureData mFlowerTextureNormal;
    agl::TextureData mFlowerStalkTextureNormal;
    agl::TextureData mGrassTextureNormals[5];
    agl::TextureData _3A6C;
    agl::TextureData mGrassTextures[5];
    u8 _3E14[0x38];
    u8 _3E4C[0x30];
    u8 _3E7C[0xC];
    u8 _3E88[0x8];
};