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

namespace TextureRenderer {
    void loadTexture(
        const sead::SafeString& archiveResName,
        const sead::SafeString& textureName,
        void* textureData,  // agl::TextureData*
        void* pRes,         // nw::g3d::res::ResFile**
        void* sampler       // agl::TextureSampler*
    );
}

namespace TexQuadGrass {
    void create(
        void*,
        agl::TextureData*,
        agl::TextureData*,
        agl::TextureData*,
        agl::TextureData*,
        agl::TextureData*,
        agl::TextureData*,
        agl::TextureData*,
        agl::TextureData*,
        agl::TextureData*,
        agl::TextureData*,
        int,
        int
    );
}

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

    sead::SafeArray<agl::TextureData*, 5> mFlowerTextures;
    nw::g3d::ResFile* mResFile;
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

    mResFile = nullptr;

    for (u32 i = 0; i < 5; i++) {
        mFlowerTextures[i] = new agl::TextureData;
    }

    sead::CurrentHeapSetter chs(red::RedCoreHeap::instance());

    ResMgr::instance()->loadArchiveRes("obj_hana_kogen", "actor/obj_hana_kogen.szs", nullptr, true);

    for (u32 i = 0; i < 5; i++) {
        char texName[32] {};
        snprintf(texName, sizeof(texName), "obj_hana_kogen%02d", i + 1);

        TextureRenderer::loadTexture(
            "obj_hana_kogen",
            texName,
            mFlowerTextures[i],
            &mResFile,
            nullptr
        );        
    }

    return cResult_Success;
}

void createTexQuadGrass(
    void* texQuadGrass,
    agl::TextureData* tex1,
    agl::TextureData* tex2,
    agl::TextureData* tex3,
    agl::TextureData* tex4,
    agl::TextureData* tex5,
    agl::TextureData* nml1,
    agl::TextureData* nml2,
    agl::TextureData* nml3,
    agl::TextureData* nml4,
    agl::TextureData* nml5,
    int unk1,
    int unk2
) {
    if (FlowerManager::sInstance == nullptr) {
        // use the original textures
        TexQuadGrass::create(
            texQuadGrass,
            tex1,
            tex2,
            tex3,
            tex4,
            tex5,
            nml1,
            nml2,
            nml3,
            nml4,
            nml5,
            unk1,
            unk2
        );
        return;
    }

    FlowerManager* mgr = FlowerManager::sInstance;

    TexQuadGrass::create(
        texQuadGrass,
        mgr->mFlowerTextures[0],
        mgr->mFlowerTextures[1],
        mgr->mFlowerTextures[2],
        mgr->mFlowerTextures[3],
        mgr->mFlowerTextures[4],
        nml1,
        nml2,
        nml3,
        nml4,
        nml5,
        unk1,
        unk2
    );
}

tBranch(0x0268A620, createTexQuadGrass, tk::BranchType::bl);

} // namespace ucology
