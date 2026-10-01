#include <telkin/Telkin.h>

#include <actor/Actor.h>
#include <ucology/Ucology.h>
#include <red/util/SpriteUtil.h>
#include <map/Bg.h>
#include <system/ResMgr.h>

#include <red/heap/RedCoreHeap.h>
#include <common/aglTextureData.h>
#include <nw/g3d.h>
#include <heap/seadHeapMgr.h>


namespace ucology {

class DecorationManager : public Actor {
    SEAD_RTTI_OVERRIDE(DecorationManager, Actor);
public:
    static Profile* sProfile;
    static const ActorCreateInfo cCreateInfo;
    static DecorationManager* sInstance;
    static DecorationManager* instance() {
        return sInstance;
    }

    DecorationManager(const ActorCreateParam &param);
    ~DecorationManager() override;

    Result create() override;
    
    void initialize(BgDeco* deco);
    void loadFlowers(BgDeco* deco);
    void loadGrass(BgDeco* deco);
    void loadButterflies(BgDeco* deco);

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

SEAD_RTTI_OVERRIDE_IMPL(DecorationManager, Actor);

const ActorCreateInfo DecorationManager::cCreateInfo = {
    .flag = ActorCreateInfo::cFlag_IgnoreSpawnRange
};

Profile* DecorationManager::sProfile =
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

void DecorationManager::initialize(BgDeco* deco) {
    for (u32 i = 0; i < 5; i++) {
        mButterflyTextures[i] = new agl::TextureData;
    }

    // swap to a bigger heap
    // todo: find a more appropriate heap to use
    sead::CurrentHeapSetter chs(red::RedCoreHeap::instance());

    deco->setResFile(nullptr);
    
    BgDeco::DecorationSettings& settings = deco->getDecorationSettings();

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

    loadFlowers(deco);
    loadGrass(deco);
    loadButterflies(deco);
    
    
    deco->updateGrassAndFlowers(true);
}

void DecorationManager::loadFlowers(BgDeco* deco) {
    deco->getDecorationSettings().has_big_flowers = mHasBigFlowers;

    nw::g3d::res::ResFile* res = nullptr; // dummy;

    ResMgr::instance()->loadArchiveRes("uco_flower", "actor/uco_flower.szs", nullptr, true);

    char textureName[32];

    // flower heads
    for (u32 i = 0; i < 5; i++) {
        snprintf(textureName, sizeof(textureName), "flower_%02d_%02d", mFlowerSet, mFlowerTypes[i]);
        TextureRenderer::loadTexture("uco_flower", textureName, &deco->getFlowerTexture(i), res, nullptr);
    }

    snprintf(textureName, sizeof(textureName), "flower_%02d_nml", mFlowerSet);
    TextureRenderer::loadTexture("uco_flower", textureName, &deco->getFlowerTextureNormal(), res, nullptr);

    // it's possible to have different flower shapes within a set,
    // though it would probably be wasteful to have duplicates.
    // so for now it should be assumed that any flowers within a set
    // have the same shape
    deco->getFlowerRenderer().create(
        &deco->getFlowerTexture(0),
        &deco->getFlowerTexture(1),
        &deco->getFlowerTexture(2),
        &deco->getFlowerTexture(3),
        &deco->getFlowerTexture(4),
        &deco->getFlowerTextureNormal(),
        &deco->getFlowerTextureNormal(),
        &deco->getFlowerTextureNormal(),
        &deco->getFlowerTextureNormal(),
        &deco->getFlowerTextureNormal(),
        3,
        -1
    );

    deco->getFlowerRenderer().setDecorationType(TexQuadDeco::cDecoration_Flower);

    // flower stalks

    snprintf(textureName, sizeof(textureName), "flower_%02d_stalk", mFlowerSet);
    TextureRenderer::loadTexture("uco_flower", textureName, &deco->getFlowerStalkTexture(), res, nullptr);
    snprintf(textureName, sizeof(textureName), "flower_%02d_stalk_nml", mFlowerSet);
    TextureRenderer::loadTexture("uco_flower", textureName, &deco->getFlowerStalkTextureNormal(), res, nullptr);

    deco->getFlowerStalkRenderer().create(
        &deco->getFlowerStalkTexture(),
        &deco->getFlowerStalkTexture(),
        &deco->getFlowerStalkTexture(),
        &deco->getFlowerStalkTexture(),
        &deco->getFlowerStalkTexture(),
        &deco->getFlowerStalkTextureNormal(),
        &deco->getFlowerStalkTextureNormal(),
        &deco->getFlowerStalkTextureNormal(),
        &deco->getFlowerStalkTextureNormal(),
        &deco->getFlowerStalkTextureNormal(),
        3,
        -1
    );

    deco->getFlowerStalkRenderer().setDecorationType(TexQuadDeco::cDecoration_FlowerStem);
}

void DecorationManager::loadGrass(BgDeco* deco) {
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
                TextureRenderer::loadTexture("obj_kusa_chika", textureName, &deco->getGrassTexture(i), res, nullptr);

                snprintf(textureName, sizeof(textureName), "obj_kusa_chika%02d_nml", i + 1);
                TextureRenderer::loadTexture("obj_kusa_chika", textureName, &deco->getGrassTextureNormal(i), res, nullptr);
            }
            break;
        }

        // sky
        case 2: {
            ResMgr::instance()->loadArchiveRes("obj_kusa_kogen", "actor/obj_kusa_kogen.szs", nullptr, true);
            for (u32 i = 0; i < 5; i++) {
                snprintf(textureName, sizeof(textureName), "obj_kusa_kogen%02d", i + 1);
                TextureRenderer::loadTexture("obj_kusa_kogen", textureName, &deco->getGrassTexture(i), res, nullptr);

                snprintf(textureName, sizeof(textureName), "obj_kusa_kogen%02d_nml", i + 1);
                TextureRenderer::loadTexture("obj_kusa_kogen", textureName, &deco->getGrassTextureNormal(i), res, nullptr);
            }
            break;
        }

        // forest
        case 3: {
            ResMgr::instance()->loadArchiveRes("obj_kusa_daishizen", "actor/obj_kusa_daishizen.szs", nullptr, true);
            for (u32 i = 0; i < 5; i++) {
                snprintf(textureName, sizeof(textureName), "obj_kusa_dai%02d", i + 1);
                TextureRenderer::loadTexture("obj_kusa_daishizen", textureName, &deco->getGrassTexture(i), res, nullptr);

                snprintf(textureName, sizeof(textureName), "obj_kusa_dai%02d_nml", i + 1);
                TextureRenderer::loadTexture("obj_kusa_daishizen", textureName, &deco->getGrassTextureNormal(i), res, nullptr);
            }
            break;
        }

        // standard
        case 0:
        default: {
            ResMgr::instance()->loadArchiveRes("obj_kusa", "actor/obj_kusa.szs", nullptr, true);

            for (u32 i = 0; i < 5; i++) {
                snprintf(textureName, sizeof(textureName), "obj_kusa%02d", i + 1);
                TextureRenderer::loadTexture("obj_kusa", textureName, &deco->getGrassTexture(i), res, nullptr);

                snprintf(textureName, sizeof(textureName), "obj_kusa%02d_nml", i + 1);
                TextureRenderer::loadTexture("obj_kusa", textureName, &deco->getGrassTextureNormal(i), res, nullptr);
            }
        }
    }
    
    deco->getGrassRenderer().create(
        &deco->getGrassTexture(0),
        &deco->getGrassTexture(1),
        &deco->getGrassTexture(2),
        &deco->getGrassTexture(3),
        &deco->getGrassTexture(4),
        &deco->getGrassTextureNormal(0),
        &deco->getGrassTextureNormal(1),
        &deco->getGrassTextureNormal(2),
        &deco->getGrassTextureNormal(3),
        &deco->getGrassTextureNormal(4),
        3,
        -1
    );
    
    deco->getGrassRenderer().setDecorationType(TexQuadDeco::cDecoration_Grass);
}

void DecorationManager::loadButterflies(BgDeco* deco) {
    deco->getDecorationSettings().has_butterflies = !mDisableButterflies;

    nw::g3d::res::ResFile* res = nullptr; // dummy

    ResMgr::instance()->loadArchiveRes("uco_butterfly", "actor/uco_butterfly.szs", nullptr, true);

    char textureName[32];
    for (u32 i = 0; i < 5; i++) {
        snprintf(textureName, sizeof(textureName), "butterfly_%02d", mButterflyTypes[i]);
        TextureRenderer::loadTexture("uco_butterfly", textureName, mButterflyTextures[i], res, nullptr);
    }

    deco->getButterflyRenderer().create(
        mButterflyTextures[0],
        mButterflyTextures[1],
        mButterflyTextures[2],
        mButterflyTextures[3],
        mButterflyTextures[4],
        &deco->getGrassTextureNormal(0), // this is what the original game does
        &deco->getGrassTextureNormal(0),
        &deco->getGrassTextureNormal(0),
        &deco->getGrassTextureNormal(0),
        &deco->getGrassTextureNormal(0),
        3,
        -1
    );

    deco->getButterflyRenderer().setDecorationType(TexQuadDeco::cDecoration_Butterfly);
}

void initializeDecoration(BgDeco* deco) {
    DecorationManager* inst = DecorationManager::sInstance;
    if (inst == nullptr) {
        deco->initialize();
        return;
    }

    inst->initialize(deco);
}


tBranch(0x0268B678, initializeDecoration, tk::BranchType::b);

} // namespace ucology
