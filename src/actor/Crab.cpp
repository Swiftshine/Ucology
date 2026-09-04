#include <actor/ActorState.h>
#include <effect/EffectCreateUtil.h>
#include <game/Quake.h>
#include <graphics/AnimModel.h>
#include <random/seadGlobalRandom.h>
#include <ucology/Ucology.h>

namespace ucology {

class Crab : public ActorMultiState {
public:
    static Profile* cProfile;
    static const ActorCreateInfo cCreateInfo;
    static const ActorBgCollisionCheck::Sensor cBottomSensor;
    static const ActorBgCollisionCheck::Sensor cTopSensor;
    static const ActorBgCollisionCheck::Sensor cAdjacentSensor;

    Crab(const ActorCreateParam& param);
    ~Crab() override = default;

    Result create() override;
    bool preExecute() override;
    bool execute() override;
    bool draw() override;
    
    DECLARE_STATE_ID(Crab, Idle);
    DECLARE_STATE_ID(Crab, Walk);
    DECLARE_STATE_ID(Crab, Burrow);

    void updateModel() const {
        constexpr f32 SCALE_FACTOR = 0.16f;
        static const sead::Vector3f MODEL_SCALE(SCALE_FACTOR, SCALE_FACTOR, SCALE_FACTOR);
        mModel->update(mPos, mAngle, MODEL_SCALE);
    }

    bool isPlayerClose() {
        const float PLAYER_DISTANCE_THRESHOLD = 16.0f * 5.0f;

        sead::Vector2f dist;
        if (searchNearPlayer(dist) == -1) {
            // no player found
            return false;
        }

        return dist.length() <= PLAYER_DISTANCE_THRESHOLD;
    }

    void changeStateIfPlayerClose() {
        if (isPlayerClose()) {
            changeState(StateID_Burrow);
        }
    }
private:
    // graphics
    AnimModel* mModel;
    bool mStartIdleAnimationFromBeginning;

    // burrow
    bool mWasQuaked;
};

const ActorCreateInfo Crab::cCreateInfo = {
    .offset_x = 8, .offset_y = 8
};

Profile* Crab::cProfile = ucology::getRegistrar()->newProfile<Crab>("crab")
    .resources<"uco_crab">(ProfileInfo::cResType_Course)
    .flag(Profile::cFlag_DrawCullCheck)
    .createInfo(&cCreateInfo)
    .build();

const ActorBgCollisionCheck::Sensor Crab::cBottomSensor = ActorBgCollisionCheck::Sensor(-3.0, 3.0, 0.0);
const ActorBgCollisionCheck::Sensor Crab::cTopSensor = ActorBgCollisionCheck::Sensor(-3.0, 3.0, 16.0);
const ActorBgCollisionCheck::Sensor Crab::cAdjacentSensor = ActorBgCollisionCheck::Sensor(5.0, 11.0, 8.0);

CREATE_STATE_ID(Crab, Idle);
CREATE_STATE_ID(Crab, Walk);
CREATE_STATE_ID(Crab, Burrow);

Crab::Crab(const ActorCreateParam& param)
    : ActorMultiState(param)
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

    // physics
    mSpeedMax = sead::Vector3f(1.0f, -4.0f, 0.0f);
    mAccelY = -0.1875f;
    mWasQuaked = false;

    changeState(StateID_Idle);
    return cResult_Success;
}

bool Crab::preExecute() {
    u32 quakeFlag = Quake::instance()->getFlag();

    bool pow = (quakeFlag & 8) != 0;
    bool multiGroundPound = (quakeFlag & 0x10) != 0;

    if (pow || multiGroundPound) {
        mWasQuaked = true;
    }

    return true;
}

bool Crab::execute() {
    if (mStateMgr.getStateID() != &StateID_Burrow) {
        calcSpeedY_();
        posMove_();
        mBgCheckObj.checkBg();
        
        if (mWasQuaked) {
            // wait until we're on the floor again to fly away
            if (mBgCheckObj.getOutput().checkFoot()) {
                changeState(StateID_Burrow);
            }
        }

        if (mBgCheckObj.checkWall(DirType::cDirType_Left)) {
            mDirection = DirType::cDirType_Right;
        }

        if (mBgCheckObj.checkWall(DirType::cDirType_Right)) {
            mDirection = DirType::cDirType_Left;
        }
    }

    updateModel();
    executeState();

    if (mStateMgr.getStateID() != &StateID_Burrow) {
        changeStateIfPlayerClose();
    }

    return true;
}

bool Crab::draw() {
    mModel->draw();
    return true;
}

void Crab::initializeState_Idle() {
    mModel->playSklAnim("Wait");
    mModel->getSklAnim(0)->getFrameCtrl().setPlayMode(FrameCtrl::cMode_Repeat);

    mStartIdleAnimationFromBeginning = true;
}

void Crab::executeState_Idle() {
    // 1 in n chance every frame to start walking in any direction
    constexpr u32 WALK_CHANCE = 50;
    if (sead::GlobalRandom::instance()->getU32(WALK_CHANCE) == 1) {
        mDirection = static_cast<DirType>(sead::GlobalRandom::instance()->getBool());
        changeState(StateID_Walk);
    }

    // if we haven't moved yet then make sure we don't go back to the first frame
    constexpr u32 WAIT_ANIM_LOOP_FRAME_START = 15;
    if (mStartIdleAnimationFromBeginning) {
        mModel->getSklAnim(0)->getFrameCtrl().setFrameStart(WAIT_ANIM_LOOP_FRAME_START);
        mStartIdleAnimationFromBeginning = false;
    }
}

void Crab::finalizeState_Idle() { }

void Crab::initializeState_Walk() {
    mModel->playSklAnim("Run");
    mModel->getSklAnim(0)->getFrameCtrl().setPlayMode(FrameCtrl::cMode_Repeat);
}

void Crab::executeState_Walk() {
    // 1 in n chance every frame to switch back to the idle state
    constexpr u32 IDLE_CHANCE = 80;
    if (sead::GlobalRandom::instance()->getU32(IDLE_CHANCE) == 1) {
        changeState(StateID_Idle);
    }

    f32 offset = mDirection == DirType::cDirType_Right ? 1.0f : -1.0f;
    mPos.x += offset;
}

void Crab::finalizeState_Walk() { }

void Crab::initializeState_Burrow() {
    mModel->playSklAnim("Disappear");
    mModel->getSklAnim(0)->getFrameCtrl().setPlayMode(FrameCtrl::cMode_NoRepeat);
    mPos.z = -1000.0f;
}

void Crab::executeState_Burrow() {
    if (mModel->getSklAnim(0)->getFrameCtrl().isStop()) {
        BgUnitCode::Attr attr = BgUnitCode::getAttr(mBgCheckObj.getBgCheckData(DirType::cDirType_Down));
        
        switch (attr) {
            case BgUnitCode::cSand:
            case BgUnitCode::cBeachSand: {
                EffectCreateUtil::createEffect(RP_Cmn_LandingSand_01, &mPos);
                break;
            }

            default: {
                EffectCreateUtil::createEffect(RP_Cmn_LandingSmoke_00, &mPos);
                break;
            }
        }
        mDeleteRequestFlag = true;
    }
}

void Crab::finalizeState_Burrow() { }

}
