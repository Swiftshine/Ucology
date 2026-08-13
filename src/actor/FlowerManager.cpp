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

class FlowerManager : public Actor {
public:
    static Profile* cProfile;
    static FlowerManager* sInstance;
    static FlowerManager* instance() {
        return sInstance;
    }

    FlowerManager(const ActorCreateParam &param);
    ~FlowerManager() override;

    Result create() override;

    void initialize(FlowerTexMgr* texMgr);

    sead::SafeArray<agl::TextureData*, 5> mButterflyTextures;
};

Profile* FlowerManager::cProfile =
    ucology::getRegistrar()
        ->newProfile<FlowerManager>("flower_manager")
        .build();


FlowerManager* FlowerManager::sInstance = nullptr;

FlowerManager::FlowerManager(const ActorCreateParam &param)
    : Actor(param)
{ }

FlowerManager::~FlowerManager() {
    if (sInstance == this) {
        sInstance = nullptr;
    }
}

ActorBase::Result FlowerManager::create() {
    if (instance() != nullptr) {
        mDeleteRequestFlag = true;
        return cResult_Success;
    }

    sInstance = this;

    return cResult_Success;
}

void FlowerManager::initialize(FlowerTexMgr* texMgr) {
    for (u32 i = 0; i < 5; i++) {
        mButterflyTextures[i] = new agl::TextureData;
    }
    
    // todo: use ActorAdditionalHeap
    sead::CurrentHeapSetter chs(red::RedCoreHeap::instance());
    

    // initial setup

    nw::g3d::ResFile** res = &texMgr->mResFile;
    FlowerTexMgr::DecorationSettings& settings = texMgr->mSettings;
    texMgr->mResFile = nullptr;

    settings._36 = false;
    settings._c = 0.0f;
    settings._33 = false;
    settings._35 = false;
    settings.mIsBig = false;
    settings.mHasButterflies = true;
    settings._30 = 0;

    for (u32 i = 0; i < 4; i++) {
        settings._0[i + 8] = 0;
    }

    // force load our custom textures

    ResMgr::instance()->loadArchiveRes("obj_hana_custom", "actor/obj_hana_custom.szs", nullptr, true);
    
    TextureRenderer::loadTexture("obj_hana_custom", "flower_0_000", &texMgr->mFlowerTextures[0], res, nullptr);
    TextureRenderer::loadTexture("obj_hana_custom", "flower_0_001", &texMgr->mFlowerTextures[1], res, nullptr);
    TextureRenderer::loadTexture("obj_hana_custom", "flower_0_002", &texMgr->mFlowerTextures[2], res, nullptr);
    TextureRenderer::loadTexture("obj_hana_custom", "flower_0_000", &texMgr->mFlowerTextures[3], res, nullptr);
    TextureRenderer::loadTexture("obj_hana_custom", "flower_0_000", &texMgr->mFlowerTextures[4], res, nullptr);
    TextureRenderer::loadTexture("obj_hana_custom", "flower_0_xxx_nml", &texMgr->mFlowerTextureNormal, res, nullptr);
    

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
    
    TextureRenderer::loadTexture("obj_hana_custom", "stem_0_000", &texMgr->mFlowerStalkTexture, res, nullptr);
    TextureRenderer::loadTexture("obj_hana_custom", "stem_0_xxx_nml", &texMgr->mFlowerStalkTextureNormal, res, nullptr);

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

    texMgr->mFlowerStalkRenderer.mDecorationType = TexQuadGrass::DecorationType::cDecoration_FlowerStem;

    ResMgr::instance()->loadArchiveRes("obj_kusa", "actor/obj_kusa.szs", nullptr, true);
    TextureRenderer::loadTexture("obj_kusa", "obj_kusa01", &texMgr->mGrassTextures[0], res, nullptr);
    TextureRenderer::loadTexture("obj_kusa", "obj_kusa02", &texMgr->mGrassTextures[1], res, nullptr);
    TextureRenderer::loadTexture("obj_kusa", "obj_kusa03", &texMgr->mGrassTextures[2], res, nullptr);
    TextureRenderer::loadTexture("obj_kusa", "obj_kusa04", &texMgr->mGrassTextures[3], res, nullptr);
    TextureRenderer::loadTexture("obj_kusa", "obj_kusa05", &texMgr->mGrassTextures[4], res, nullptr);
    
    TextureRenderer::loadTexture("obj_kusa", "obj_kusa01_nml", &texMgr->mGrassTextureNormals[0], res, nullptr);
    TextureRenderer::loadTexture("obj_kusa", "obj_kusa02_nml", &texMgr->mGrassTextureNormals[1], res, nullptr);
    TextureRenderer::loadTexture("obj_kusa", "obj_kusa03_nml", &texMgr->mGrassTextureNormals[2], res, nullptr);
    TextureRenderer::loadTexture("obj_kusa", "obj_kusa04_nml", &texMgr->mGrassTextureNormals[3], res, nullptr);
    TextureRenderer::loadTexture("obj_kusa", "obj_kusa05_nml", &texMgr->mGrassTextureNormals[4], res, nullptr);
    
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

    TextureRenderer::loadTexture("obj_hana_custom", "butterfly_000", mButterflyTextures[0], res, nullptr);
    TextureRenderer::loadTexture("obj_hana_custom", "butterfly_001", mButterflyTextures[1], res, nullptr);
    TextureRenderer::loadTexture("obj_hana_custom", "butterfly_002", mButterflyTextures[2], res, nullptr);
    TextureRenderer::loadTexture("obj_hana_custom", "butterfly_003", mButterflyTextures[3], res, nullptr);
    TextureRenderer::loadTexture("obj_hana_custom", "butterfly_004", mButterflyTextures[4], res, nullptr);

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

    settings._2c = 0;
    settings._34 = false;
    
    texMgr->mButterflyRenderer.mDecorationType = TexQuadGrass::DecorationType::cDecoration_Butterfly;

    texMgr->updateGrassAndFlowers(true);
}

void initializeFlowers(FlowerTexMgr* texMgr) {
    if (FlowerManager::sInstance == nullptr) {
        // let the original code handle it
        texMgr->initialize();
        return;
    }

    FlowerManager::sInstance->initialize(texMgr);

}


tBranch(0x0268B678, initializeFlowers, tk::BranchType::b);

} // namespace ucology
