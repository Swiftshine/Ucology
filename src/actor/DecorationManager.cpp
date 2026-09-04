#include <telkin/Telkin.h>

#include <actor/Actor.h>
#include <ucology/Ucology.h>
#include <red/util/SpriteUtil.h>
#include <map/Bg.h>
#include <system/ResMgr.h>
#include <graphics/FlowerTexMgr.h> 

#include <red/heap/RedCoreHeap.h>
#include <common/aglTextureData.h>
#include <nw/g3d.h>
#include <heap/seadHeapMgr.h>


namespace ucology {

class DecorationManager : public Actor {
public:
    static Profile* cProfile;
    static const ActorCreateInfo cCreateInfo;
    static DecorationManager* sInstance;
    static DecorationManager* instance() {
        return sInstance;
    }

    DecorationManager(const ActorCreateParam &param);
    ~DecorationManager() override;

    Result create() override;

    void initialize(FlowerTexMgr* texMgr);
    void loadFlowers(FlowerTexMgr* texMgr);
    void loadGrass(FlowerTexMgr* texMgr);
    void loadButterflies(FlowerTexMgr* texMgr);

    u8 mDecorationSet;
    bool mHasBigFlowers;
    bool mDisableButterflies;
    u8 mFlowerSet;
    // u8 mGrassSet;
    u8 mFlowerTypes[5];
    u8 mGrassType;
    u8 mButterflyTypes[5];
    sead::SafeArray<agl::TextureData*, 5> mButterflyTextures;
};

const ActorCreateInfo DecorationManager::cCreateInfo = {
    .flag = ActorCreateInfo::cFlag_IgnoreSpawnRange
};

Profile* DecorationManager::cProfile =
    ucology::getRegistrar()
        ->newProfile<DecorationManager>("decoration_manager")
        .createInfo(&cCreateInfo)
        .build();

DecorationManager* DecorationManager::sInstance = nullptr;

DecorationManager::DecorationManager(const ActorCreateParam &param)
    : Actor(param)
{ }

DecorationManager::~DecorationManager() {
    if (sInstance == this) {
        sInstance = nullptr;
    }
}

ActorBase::Result DecorationManager::create() {
    if (instance() != nullptr) {
        mDeleteRequestFlag = true;
        return cResult_Success;
    }

    u8 decorationSet = red::SpriteUtil::getNybble1(this);
    
    if (decorationSet < 6) {
        mDeleteRequestFlag = true;
        return cResult_Success;
    }

    mDecorationSet = decorationSet;

    u8 nybble2 = red::SpriteUtil::getNybble2(this);
    mHasBigFlowers = (nybble2 & 1) != 0;
    mDisableButterflies = (nybble2 & 2) != 0;

    mFlowerSet = red::SpriteUtil::getNybble3(this);
    // mGrassSet = red::SpriteUtil::getNybble4(this);
    mFlowerTypes[0] = red::SpriteUtil::getNybble5(this);
    mFlowerTypes[1] = red::SpriteUtil::getNybble6(this);
    mFlowerTypes[2] = red::SpriteUtil::getNybble7(this);
    mFlowerTypes[3] = red::SpriteUtil::getNybble8(this);
    mFlowerTypes[4] = red::SpriteUtil::getNybble9(this);
    mGrassType = red::SpriteUtil::getNybble10(this);
    mButterflyTypes[0] = red::SpriteUtil::getNybble11(this);
    mButterflyTypes[1] = red::SpriteUtil::getNybble12(this);
    mButterflyTypes[2] = red::SpriteUtil::getNybble13(this);
    mButterflyTypes[3] = red::SpriteUtil::getNybble14(this);
    mButterflyTypes[4] = red::SpriteUtil::getNybble15(this);
    sInstance = this;
    return cResult_Success;
}

void DecorationManager::initialize(FlowerTexMgr* texMgr) {
    for (u32 i = 0; i < 5; i++) {
        mButterflyTextures[i] = new agl::TextureData;
    }

    // swap to a bigger heap
    // todo: use ActorAdditionalHeap
    sead::CurrentHeapSetter chs(red::RedCoreHeap::instance());

    nw::g3d::ResFile** res = &texMgr->mResFile;
    FlowerTexMgr::DecorationSettings& settings = texMgr->mSettings;
    texMgr->mResFile = nullptr;

    settings._36 = false;
    settings._c = 0.0f;
    settings._33 = false;
    settings._35 = false;
    settings._30 = 0;
    settings._2c = 0;
    settings._34 = false;

    for (u32 i = 0; i < 4; i++) {
        settings._0[i + 8] = 0;
    }

    loadFlowers(texMgr);
    loadGrass(texMgr);
    loadButterflies(texMgr);
    
    
    texMgr->updateGrassAndFlowers(true);
}

void DecorationManager::loadFlowers(FlowerTexMgr* texMgr) {
    texMgr->mSettings.mHasBigFlowers = mHasBigFlowers;
    nw::g3d::res::ResFile* res = nullptr; // dummy;

    ResMgr::instance()->loadArchiveRes("uco_flower", "actor/uco_flower.szs", nullptr, true);

    char textureName[32];

    // flower heads
    for (u32 i = 0; i < 5; i++) {
        snprintf(textureName, sizeof(textureName), "flower_%02d_%02d", mFlowerSet, mFlowerTypes[i]);
        TextureRenderer::loadTexture("uco_flower", textureName, &texMgr->mFlowerTextures[i], res, nullptr);
    }

    snprintf(textureName, sizeof(textureName), "flower_%02d_nml", mFlowerSet);
    TextureRenderer::loadTexture("uco_flower", textureName, &texMgr->mFlowerTextureNormal, res, nullptr);

    // it's possible to have different flower shapes within a set,
    // though it would probably be wasteful to have duplicates.
    // so for now it should be assumed that any flowers within a set
    // have the same shape
    texMgr->mFlowerRenderer.create( 
        &texMgr->mFlowerTextures[0],
        &texMgr->mFlowerTextures[1],
        &texMgr->mFlowerTextures[2],
        &texMgr->mFlowerTextures[3],
        &texMgr->mFlowerTextures[4],
        &texMgr->mFlowerTextureNormal,
        &texMgr->mFlowerTextureNormal,
        &texMgr->mFlowerTextureNormal,
        &texMgr->mFlowerTextureNormal,
        &texMgr->mFlowerTextureNormal,
        3,
        -1
    );

    texMgr->mFlowerRenderer.mDecorationType = TexQuadGrass::DecorationType::cDecoration_Flower;

    // flower stalks
    snprintf(textureName, sizeof(textureName), "flower_%02d_stalk", mFlowerSet);
    TextureRenderer::loadTexture("uco_flower", textureName, &texMgr->mFlowerStalkTexture, res, nullptr);
    snprintf(textureName, sizeof(textureName), "flower_%02d_stalk_nml", mFlowerSet);
    TextureRenderer::loadTexture("uco_flower", textureName, &texMgr->mFlowerStalkTextureNormal, res, nullptr);

    texMgr->mFlowerStalkRenderer.create(
        &texMgr->mFlowerStalkTexture,
        &texMgr->mFlowerStalkTexture,
        &texMgr->mFlowerStalkTexture,
        &texMgr->mFlowerStalkTexture,
        &texMgr->mFlowerStalkTexture,
        &texMgr->mFlowerStalkTextureNormal,
        &texMgr->mFlowerStalkTextureNormal,
        &texMgr->mFlowerStalkTextureNormal,
        &texMgr->mFlowerStalkTextureNormal,
        &texMgr->mFlowerStalkTextureNormal,
        3,
        -1
    );

    texMgr->mFlowerStalkRenderer.mDecorationType = TexQuadGrass::cDecoration_FlowerStem;
}

void DecorationManager::loadGrass(FlowerTexMgr* texMgr) {
    nw::g3d::res::ResFile* res = nullptr; // dummy;

    // todo: allow the user to select custom grass types
    // for now, though, just the vanilla ones

    char textureName[32];

    switch (mGrassType) {
        
        // underground
        case 1: {
            ResMgr::instance()->loadArchiveRes("obj_kusa_chika", "actor/obj_kusa_chika.szs", nullptr, true);
            for (u32 i = 0; i < 5; i++) {
                snprintf(textureName, sizeof(textureName), "obj_kusa_chika%02d", i + 1);
                TextureRenderer::loadTexture("obj_kusa_chika", textureName, &texMgr->mGrassTextures[i], res, nullptr);

                snprintf(textureName, sizeof(textureName), "obj_kusa_chika%02d_nml", i + 1);
                TextureRenderer::loadTexture("obj_kusa_chika", textureName, &texMgr->mGrassTextureNormals[i], res, nullptr);
            }
            break;
        }

        // sky
        case 2: {
            ResMgr::instance()->loadArchiveRes("obj_kusa_kogen", "actor/obj_kusa_kogen.szs", nullptr, true);
            for (u32 i = 0; i < 5; i++) {
                snprintf(textureName, sizeof(textureName), "obj_kusa_kogen%02d", i + 1);
                TextureRenderer::loadTexture("obj_kusa_kogen", textureName, &texMgr->mGrassTextures[i], res, nullptr);

                snprintf(textureName, sizeof(textureName), "obj_kusa_kogen%02d_nml", i + 1);
                TextureRenderer::loadTexture("obj_kusa_kogen", textureName, &texMgr->mGrassTextureNormals[i], res, nullptr);
            }
            break;
        }

        // forest
        case 3: {
            ResMgr::instance()->loadArchiveRes("obj_kusa_daishizen", "actor/obj_kusa_daishizen.szs", nullptr, true);
            for (u32 i = 0; i < 5; i++) {
                snprintf(textureName, sizeof(textureName), "obj_kusa_dai%02d", i + 1);
                TextureRenderer::loadTexture("obj_kusa_daishizen", textureName, &texMgr->mGrassTextures[i], res, nullptr);

                snprintf(textureName, sizeof(textureName), "obj_kusa_dai%02d_nml", i + 1);
                TextureRenderer::loadTexture("obj_kusa_daishizen", textureName, &texMgr->mGrassTextureNormals[i], res, nullptr);
            }
            break;
        }

        // standard
        case 0:
        default: {
            ResMgr::instance()->loadArchiveRes("obj_kusa", "actor/obj_kusa.szs", nullptr, true);

            for (u32 i = 0; i < 5; i++) {
                snprintf(textureName, sizeof(textureName), "obj_kusa%02d", i + 1);
                TextureRenderer::loadTexture("obj_kusa", textureName, &texMgr->mGrassTextures[i], res, nullptr);

                snprintf(textureName, sizeof(textureName), "obj_kusa%02d_nml", i + 1);
                TextureRenderer::loadTexture("obj_kusa", textureName, &texMgr->mGrassTextureNormals[i], res, nullptr);
            }
        }
    }
    

    texMgr->mGrassRenderer.create(
        &texMgr->mGrassTextures[0],
        &texMgr->mGrassTextures[1],
        &texMgr->mGrassTextures[2],
        &texMgr->mGrassTextures[3],
        &texMgr->mGrassTextures[4],
        &texMgr->mGrassTextureNormals[0],
        &texMgr->mGrassTextureNormals[1],
        &texMgr->mGrassTextureNormals[2],
        &texMgr->mGrassTextureNormals[3],
        &texMgr->mGrassTextureNormals[4],
        3,
        -1
    );

    texMgr->mGrassRenderer.mDecorationType = TexQuadGrass::DecorationType::cDecoration_Grass;
}

void DecorationManager::loadButterflies(FlowerTexMgr* texMgr) {
    texMgr->mSettings.mHasButterflies = !mDisableButterflies;

    nw::g3d::res::ResFile* res = nullptr; // dummy

    ResMgr::instance()->loadArchiveRes("uco_butterfly", "actor/uco_butterfly.szs", nullptr, true);

    char textureName[32];
    for (u32 i = 0; i < 5; i++) {
        snprintf(textureName, sizeof(textureName), "butterfly_%02d", mButterflyTypes[i]);
        TextureRenderer::loadTexture("uco_butterfly", textureName, mButterflyTextures[i], res, nullptr);
    }

    texMgr->mButterflyRenderer.create(
        mButterflyTextures[0],
        mButterflyTextures[1],
        mButterflyTextures[2],
        mButterflyTextures[3],
        mButterflyTextures[4],
        &texMgr->mGrassTextureNormals[0], // this is what the game does
        &texMgr->mGrassTextureNormals[1],
        &texMgr->mGrassTextureNormals[2],
        &texMgr->mGrassTextureNormals[3],
        &texMgr->mGrassTextureNormals[4],
        3,
        -1
    );

    texMgr->mButterflyRenderer.mDecorationType = TexQuadGrass::DecorationType::cDecoration_Butterfly;
}

void initializeDecoration(FlowerTexMgr* texMgr) {
    DecorationManager* deco = DecorationManager::sInstance;
    if (deco == nullptr) {
        texMgr->initialize();
        return;
    }

    deco->initialize(texMgr);
}


tBranch(0x0268B678, initializeDecoration, tk::BranchType::b);

} // namespace ucology
