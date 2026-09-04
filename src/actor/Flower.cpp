#include <actor/ActorCollision.h>
#include <ucology/Ucology.h>
#include <map/Bg.h>
#include <red/util/SpriteUtil.h>

#include <random/seadGlobalRandom.h>

namespace ucology {

class Flower : public ActorCollision {
public:
    enum FlowerType : u8 {
        A, B, C, D, E,
        Count
    };
public:
    static Profile* cProfile;
    static const ActorCreateInfo cCreateInfo;
    static const ActorBgCollisionCheck::Sensor cBottomSensor;
    static const ActorBgCollisionCheck::Sensor cTopSensor;
    static const ActorBgCollisionCheck::Sensor cAdjacentSensor;

    Flower(const ActorCreateParam& param);
    ~Flower() override = default;

    Result create() override;
    bool execute() override;
    Result doDelete() override;
    void blockHitInit_() override;

    bool isAllowedInRandomizer(u32 flowerType) const {
        return (mRandomizerFlags & (1 << flowerType)) != 0;
    }

    u8 getRandomFlowerType() const;

    u8 mFlowerIndex;
    u8 mFlowerType;
    bool mAffectedByGravity;
    bool mIsRandomized;
    u16 mRandomizerFlags;
};

const ActorCreateInfo Flower::cCreateInfo = {
    .offset_x = 8,
    .offset_y = -16,
};

Profile* Flower::cProfile = ucology::getRegistrar()->newProfile<Flower>("flower")
    .createInfo(&cCreateInfo)
    .build();

const ActorBgCollisionCheck::Sensor Flower::cBottomSensor = ActorBgCollisionCheck::Sensor(-3.0, 3.0, 0.0);
const ActorBgCollisionCheck::Sensor Flower::cTopSensor = ActorBgCollisionCheck::Sensor(-3.0, 3.0, 16.0);
const ActorBgCollisionCheck::Sensor Flower::cAdjacentSensor = ActorBgCollisionCheck::Sensor(5.0, 11.0, 8.0);

Flower::Flower(const ActorCreateParam &param) : ActorCollision(param) {}

ActorBase::Result Flower::create() {
    mFlowerIndex = Bg::instance()->getNextFlowerIndex();

    mIsRandomized = static_cast<bool>(red::SpriteUtil::getNybble1(this));
    mAffectedByGravity = static_cast<bool>(red::SpriteUtil::getNybble2(this));
    
    mRandomizerFlags = red::SpriteUtil::getNybble4(this) << 4;
    mRandomizerFlags |= red::SpriteUtil::getNybble5(this);
    
    if (mIsRandomized) {
        mFlowerType = getRandomFlowerType();
    } else {
        mFlowerType = red::SpriteUtil::getNybble3(this);
    }
    
    Bg::instance()->registerFlower(mPos.x, mPos.y, mPos.z, mFlowerType, mFlowerIndex, 0xFF);
    
    if (mAffectedByGravity) {
        mBgCheckObj.set(this, &cBottomSensor, &cTopSensor, &cAdjacentSensor);
    
        for (DirType dir : {
            DirType::cDirType_Down,
            DirType::cDirType_Up,
            DirType::cDirType_Right,
            DirType::cDirType_Left
        }) {
            auto& flag = mBgCheckObj.getSensorFlag(dir);
            flag.setBit(31);
            flag.setBit(32);
            flag.setBit(41);
            flag.setBit(49);
        }

        mSpeed = sead::Vector3f(0.0f, 0.0f, 0.0f);
        mSpeedMax = sead::Vector3f(0.0f, -4.0f, 0.0f);
        mAccelY = -0.1875f;
    }

    return cResult_Success;
}

bool Flower::execute() {
    if (!screenOutCheck(0)) {
        if (mAffectedByGravity) {
            calcSpeedY_();
            posMove_();
            mBgCheckObj.checkBg();
    
            if (mBgCheckObj.getOutput().checkFoot()) {
                mSpeed.y = 0.0f;
            }
        }

        // update graphics
        Bg::instance()->updateFlower(mPos.x, mPos.y, mFlowerIndex, true);
    }

    return true;
}

ActorBase::Result Flower::doDelete() {
    Bg::instance()->deleteFlower(mFlowerIndex);

    return cResult_Success;
}

void Flower::blockHitInit_() {
    if (mAffectedByGravity) {
        mSpeed.y = 3.5f;
    }
}

u8 Flower::getRandomFlowerType() const {
    u32 allowedCount = 0;

    for (u32 type = 0; type < FlowerType::Count; type++) {
        if (isAllowedInRandomizer(type)) {
            allowedCount++;
        }
    }

    if (allowedCount == 0) {
        return FlowerType::A;
    }

    u32 selection = sead::GlobalRandom::instance()->getU32(allowedCount);

    for (u32 type = 0; type < FlowerType::Count; type++) {
        if (isAllowedInRandomizer(type)) {
            if (selection == 0) {
                return static_cast<u8>(type);
            }

            selection--;
        }
    }

    return FlowerType::A;
}

}
