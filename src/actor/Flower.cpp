#include <telkin/Telkin.h>

#include <actor/ActorCollision.h>
#include <actor/ActorMgr.h>
#include <ucology/Ucology.h>
#include <map/Bg.h>
#include <red/util/SpriteUtil.h>
#include <utility/RotShake.h>
#include <ucology/FlowerSet.h>

#include <cstring>


namespace ucology {

class Flower : public ActorCollision {
public:
    static Profile* cProfile;
    static const ActorBgCollisionCheck::Sensor cBottomSensor;
    static const ActorBgCollisionCheck::Sensor cTopSensor;
    static const ActorBgCollisionCheck::Sensor cAdjacentSensor;

    Flower(const ActorCreateParam& param);
    ~Flower() override = default;

    Result create() override;
    bool execute() override;
    Result doDelete() override;
    void blockHitInit_() override;

    u8 mColorIndex;
    u8 mFlowerIndex;
};

Profile* Flower::cProfile = ucology::getRegistrar()->newProfile<Flower>("flower").build();

const ActorBgCollisionCheck::Sensor Flower::cBottomSensor = ActorBgCollisionCheck::Sensor(-3.0, 3.0, 0.0);
const ActorBgCollisionCheck::Sensor Flower::cTopSensor = ActorBgCollisionCheck::Sensor(-3.0, 3.0, 16.0);
const ActorBgCollisionCheck::Sensor Flower::cAdjacentSensor = ActorBgCollisionCheck::Sensor(5.0, 11.0, 8.0);

Flower::Flower(const ActorCreateParam &param) : ActorCollision(param) {}

ActorBase::Result Flower::create() {
    mFlowerIndex = Bg::instance()->getNextFlowerIndex();
    mColorIndex = red::SpriteUtil::getNybble1(this);

    mPos.x += 8.0f;
    Bg::instance()->registerFlower(mPos.x, mPos.y, mPos.z, mColorIndex, mFlowerIndex, 0xFF);

    // assume gravity is enabled

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
    
    return cResult_Success;
}

bool Flower::execute() {
    if (!screenOutCheck(0)) {
        // update collision
        calcSpeedY_();
        posMove_();
        mBgCheckObj.checkBg();

        if (mBgCheckObj.getOutput().checkFoot()) {
            mSpeed.y = 0.0f;
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
    mSpeed.y = 4.0f;
}

}
