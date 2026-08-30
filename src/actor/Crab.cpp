// wip!

#include <actor/ActorCollision.h>
#include <ucology/Ucology.h>
#include <graphics/AnimModel.h>

namespace ucology {

class Crab : public ActorCollision {
public:
    static Profile* cProfile;
    static const ActorBgCollisionCheck::Sensor cBottomSensor;
    static const ActorBgCollisionCheck::Sensor cTopSensor;
    static const ActorBgCollisionCheck::Sensor cAdjacentSensor;

    Crab(const ActorCreateParam& param);
    ~Crab() override = default;

    Result create() override;
    bool execute() override;
    bool draw() override;
private:
    AnimModel* mModel;
};

Profile* Crab::cProfile = ucology::getRegistrar()->newProfile<Crab>("crab")
    .resources<"uco_crab">(ProfileInfo::cResType_Course)
    .flag(Profile::cFlag_DrawCullCheck)
    .build();

const ActorBgCollisionCheck::Sensor Crab::cBottomSensor = ActorBgCollisionCheck::Sensor(-3.0, 3.0, 0.0);
const ActorBgCollisionCheck::Sensor Crab::cTopSensor = ActorBgCollisionCheck::Sensor(-3.0, 3.0, 16.0);
const ActorBgCollisionCheck::Sensor Crab::cAdjacentSensor = ActorBgCollisionCheck::Sensor(5.0, 11.0, 8.0);

Crab::Crab(const ActorCreateParam& param)
    : ActorCollision(param)
{ }

ActorBase::Result Crab::create() {
    // model setup
    mModel = AnimModel::create("uco_crab", "uco_crab", 3);
    mModel->playSklAnim("Run");

    // collision setup
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

    mScale = sead::Vector3f(1.0f, 1.0f, 1.0f);
    mSpeed = sead::Vector3f(0.0f, 0.0f, 0.0f);
    mSpeedMax = sead::Vector3f(1.0f, -4.0f, 0.0f);

    return cResult_Success;
}

bool Crab::execute() {
    calcSpeedX_();
    calcSpeedY_();
    posMove_();
    mBgCheckObj.checkBg();

    if (mBgCheckObj.getOutput().checkFoot()) {
        mSpeed.y = 0.0f;
    }

    mModel->update(mPos, mAngle, mScale);

    return true;
}

bool Crab::draw() {
    mModel->draw();
    return true;
}

}
