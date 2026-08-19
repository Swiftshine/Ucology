// wip!
// BUG: y position too high
// TODO: fix shading on models. all models are currently unshaded

#include <actor/ActorState.h>
#include <actor/AttentionLookat.h>
#include <actor/AttentionMgr.h>
#include <ucology/Ucology.h>
#include <ucology/Easing.h>
#include <graphics/AnimModel.h>
#include <random/seadGlobalRandom.h>
#include <red/util/SpriteUtil.h>

namespace ucology {

class Bird : public ActorMultiState {
public:
    enum BirdType : u8 {
        // WhiteBird,
        BlueJay,
        Rosefinch,
        Lorikeet,
        Parrot,
        CrestedTit,
    };

    enum AttentionSettings : u8 {
        DoNotTake,
        TakeWhileFlying,
        TakeAlways,
    };

    enum class HopState {
        Starting,
        Ending
    };
public:
    static Profile* cProfile;
    static const ActorCreateInfo cCreateInfo;
    static const ActorBgCollisionCheck::Sensor cBottomSensor;
    static const ActorBgCollisionCheck::Sensor cTopSensor;
    static const ActorBgCollisionCheck::Sensor cAdjacentSensor;

    Bird(const ActorCreateParam& param);
    ~Bird() override;

    Result create() override;
    bool execute() override;
    bool draw() override;

    DECLARE_STATE_ID(Bird, Idle);
    DECLARE_STATE_ID(Bird, ChangeDirection);
    DECLARE_STATE_ID(Bird, Hop);
    DECLARE_STATE_ID(Bird, Fly);

    void updateAttention();

    void updateModel() const {
        const float SCALE_FACTOR = 0.1f;

        sead::Vector3f scale = mScale;
        scale.multScalar(SCALE_FACTOR);
        mModel->update(mPos, mAngle, scale);
    }

    void changeDirection() {
        mDirection = static_cast<DirType>(sead::GlobalRandom::instance()->getBool());
        float angle = mDirection == cDirType_Right ? 90.0f : -90.0f;
        mAngle.y() = sead::Mathf::deg2idx(angle);
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
            changeState(StateID_Fly);
        }
    }
private:
    // model info
    AnimModel* mModel;
    BirdType mBirdType;

    // attention info
    AttentionLookat* mAttentionLookat;
    AttentionSettings mAttentionSettings;

    // hop info
    float mHopTargetX, mBaseline;
    Easer mEaser;
    HopState mHopState = HopState::Starting;
    float mLerpRatio;

    // fly info
    u32 mFlyWaitCounter;
};

const ActorCreateInfo Bird::cCreateInfo = {
    .offset_x = 8, .offset_y = 8
};

Profile* Bird::cProfile = ucology::getRegistrar()->newProfile<Bird>("bird")
    .resources<"uco_blue_jay", "uco_rosefinch", "uco_lorikeet", "uco_parrot", "uco_crested_tit">(ProfileInfo::cResType_Course)
    .flag(Profile::cFlag_DrawCullCheck)
    .createInfo(&cCreateInfo)
    .build();

const ActorBgCollisionCheck::Sensor Bird::cBottomSensor = ActorBgCollisionCheck::Sensor(-3.0, 3.0, 0.0);
const ActorBgCollisionCheck::Sensor Bird::cTopSensor = ActorBgCollisionCheck::Sensor(-3.0, 3.0, 16.0);
const ActorBgCollisionCheck::Sensor Bird::cAdjacentSensor = ActorBgCollisionCheck::Sensor(5.0, 11.0, 8.0);

CREATE_STATE_ID(Bird, Idle);
CREATE_STATE_ID(Bird, ChangeDirection);
CREATE_STATE_ID(Bird, Fly);
CREATE_STATE_ID(Bird, Hop);

Bird::Bird(const ActorCreateParam& param)
    : ActorMultiState(param)
    , mAttentionLookat(nullptr)
{ }

Bird::~Bird() { }

ActorBase::Result Bird::create() {
    // model setup
    mScale = sead::Vector3f(1.0f, 1.0f, 1.0f);
    mBirdType = static_cast<BirdType>(red::SpriteUtil::getNybble1(this));

    const char* modelNames[] = {
        "uco_blue_jay",
        "uco_rosefinch",
        "uco_lorikeet",
        "uco_parrot",
        "uco_crested_tit"
    };

    mModel = AnimModel::create(modelNames[mBirdType], modelNames[mBirdType], 3, 1);

    if (mBirdType == BirdType::CrestedTit) {
        bool spotted = static_cast<bool>(red::SpriteUtil::getNybble2(this));

        if (spotted) {
            mModel->playTexAnim("crested_tit_texture");
            mModel->getTexAnim(0)->getFrameCtrl().setFrame(1);
            mModel->getTexAnim(0)->getFrameCtrl().setRate(0.0f);
        }
    }

    changeDirection();


    // attention setup
    mAttentionSettings = static_cast<AttentionSettings>(red::SpriteUtil::getNybble3(this));

    if (mAttentionSettings != AttentionSettings::DoNotTake) {
        mAttentionLookat = new AttentionLookat(mActorUniqueID);
        AttentionMgr::instance()->entry(*mAttentionLookat);
    }

    changeState(StateID_Idle);
    return cResult_Success;
}

bool Bird::execute() {
    // todo: physics handling here

    executeState();

    updateAttention();
    updateModel();

    if (mStateMgr.getStateID() != &StateID_Fly) {
        changeStateIfPlayerClose();
    }

    return true;
}

bool Bird::draw() {
    mModel->draw();
    return true;
}

// State: Idle
void Bird::initializeState_Idle() {
    const char* WAIT_ANIM_NAME = sead::GlobalRandom::instance()->getBool() ? "GroundWaitA" : "GroundWaitB";
    mModel->playSklAnim(WAIT_ANIM_NAME);
    mModel->getSklAnim(0)->getFrameCtrl().setPlayMode(FrameCtrl::cMode_Repeat);
}

void Bird::executeState_Idle() {
    // 1 in n chance every frame to change direction
    const u32 DIRECTION_CHANGE_CHANCE = 150;
    if (sead::GlobalRandom::instance()->getU32(DIRECTION_CHANGE_CHANCE) == 1) {
        changeState(StateID_ChangeDirection);
    }

    // 1 in n chance every frame to hop
    const u32 HOP_CHANCE = 100;
    if (sead::GlobalRandom::instance()->getU32(HOP_CHANCE) == 1) {
        changeState(StateID_Hop);
    }
}

void Bird::finalizeState_Idle() { }

// State: ChangeDirection
void Bird::initializeState_ChangeDirection() {
    // todo: properly interpolate between left and right without snapping
    mDirection = static_cast<DirType>(sead::GlobalRandom::instance()->getBool());

    // play the turn animation and actually flip the direction once it's completed
    mModel->playSklAnim("Turn");
    mModel->getSklAnim(0)->getFrameCtrl().setPlayMode(FrameCtrl::cMode_NoRepeat);
}

void Bird::executeState_ChangeDirection() {
    // check if animation is done
    if (mModel->getSklAnim(0)->getFrameCtrl().isStop()) {
        changeDirection();
        changeState(StateID_Idle);
    }
}

void Bird::finalizeState_ChangeDirection() { }

// State: Hop
void Bird::initializeState_Hop() {
    const float LATERAL_HOP_DISTANCE = 4.0f;

    // todo: account for sloped terrain

    mBaseline = mPos.y;
    mHopTargetX = mPos.x + (mDirection == cDirType_Right ? 1.0f : -1.0f) * LATERAL_HOP_DISTANCE;

    mEaser.set(Easer::EaseType::SineOut, 0.0f, 1.0f, 0.05f);

    mLerpRatio = 0.0f;
    mHopState = HopState::Starting;
}

void Bird::executeState_Hop() {
    const float HOP_HEIGHT = 4.0f;

    // horizontal
    bool done = sead::Mathf::chase(&mPos.x, mHopTargetX, 0.75f);

    // vertical
    switch (mHopState) {
        case HopState::Starting: {
            bool finished = mEaser.ease(mLerpRatio);

            mPos.y = mBaseline + HOP_HEIGHT * mLerpRatio;
  
            if (finished) {
                mEaser.set(Easer::EaseType::SineIn, 0.0f, 1.0f, 0.05f);
                mLerpRatio = 0.0f;
                mHopState = HopState::Ending;
            }
            
            break;
        }

        case HopState::Ending: {
            done &= mEaser.ease(mLerpRatio);
            mPos.y = mBaseline + HOP_HEIGHT * mLerpRatio;

            break;
        }
    }

    if (done) {
        mPos.y = mBaseline;
        changeState(StateID_Idle);
    }
}

void Bird::finalizeState_Hop() { }

// State: Fly
void Bird::initializeState_Fly() {
    mModel->playSklAnim("Fly");
    mModel->getSklAnim(0)->getFrameCtrl().setPlayMode(FrameCtrl::cMode_Repeat);
    mFlyWaitCounter = 0;
}

void Bird::executeState_Fly() {
    // todo: maybe make the movement more natural?
    const u32 FLY_AFTER_FRAMES = 8;

    if (mFlyWaitCounter >= FLY_AFTER_FRAMES) {
        mPos.x += mDirection == cDirType_Right ? mSpeed.x : -mSpeed.x;
        mPos.y += mSpeed.y;
        mSpeed.x += 0.05f;
        mSpeed.y += 0.05f;
    } else {
        mFlyWaitCounter++;
    }
}

void Bird::finalizeState_Fly() { }

void Bird::updateAttention() {
        if (mAttentionLookat == nullptr) {
            return;
        }

        switch (mAttentionSettings) {
            case AttentionSettings::DoNotTake: return;
            case AttentionSettings::TakeWhileFlying: {
                if (mStateMgr.getStateID() != &StateID_Fly) {
                    return;
                }

                // intentional fallthrough
            }
            
            case AttentionSettings::TakeAlways: {
                sead::Vector2f& pos = mAttentionLookat->getPos();
                pos.x = mPos.x;
                pos.y = mPos.y;
                break;
            }
        }
    }
}
