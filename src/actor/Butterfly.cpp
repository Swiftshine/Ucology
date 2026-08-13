#include <actor/Actor.h>
#include <ucology/Ucology.h>
#include <map/Bg.h>
#include <red/util/SpriteUtil.h>

#include <random/seadGlobalRandom.h>

namespace ucology {

class Butterfly : public Actor {
public:
    enum ButterflyType : u8 {
        A, B, C, D, E,
        Count
    };
public:
    static Profile* cProfile;

    Butterfly(const ActorCreateParam& param);
    ~Butterfly() override = default;

    Result create() override;

    bool isAllowedInRandomizer(u32 butterflyType) const {
        return (mRandomizerFlags & (1 << butterflyType)) != 0;
    }

    u8 getRandomButterflyType() const;

    u8 mButterflyType;
    bool mIsRandomized;
    u16 mRandomizerFlags;
};


Profile* Butterfly::cProfile = ucology::getRegistrar()->newProfile<Butterfly>("butterfly").build();

Butterfly::Butterfly(const ActorCreateParam &param) : Actor(param) {}

ActorBase::Result Butterfly::create() {
    mIsRandomized = static_cast<bool>(red::SpriteUtil::getNybble1(this));
    
    mRandomizerFlags = red::SpriteUtil::getNybble3(this) << 4;
    mRandomizerFlags |= red::SpriteUtil::getNybble4(this);

    if (mIsRandomized) {
        mButterflyType = getRandomButterflyType();
    } else {
        mButterflyType = red::SpriteUtil::getNybble2(this);
    }

    Bg::instance()->registerButterfly(mPos.x, mPos.y, mPos.z, mButterflyType, 0xFF, 0);
    return cResult_Success;
}

u8 Butterfly::getRandomButterflyType() const {
    u32 allowedCount = 0;

    for (u32 type = 0; type < ButterflyType::Count; type++) {
        if (isAllowedInRandomizer(type)) {
            allowedCount++;
        }
    }

    if (allowedCount == 0) {
        return ButterflyType::A;
    }

    u32 selection = sead::GlobalRandom::instance()->getU32(allowedCount);

    for (u32 type = 0; type < ButterflyType::Count; type++) {
        if (isAllowedInRandomizer(type)) {
            if (selection == 0) {
                return static_cast<u8>(type);
            }

            selection--;
        }
    }

    return ButterflyType::A;
}
}
