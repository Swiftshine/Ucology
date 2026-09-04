// FUTURE: spin when:
// - touching a player with star power
// - touching a baby yoshi with star power
// - being touched by yoshi's tongue

#include <actor/ActorMgr.h>
#include <actor/ActorState.h>
#include <actor/AttentionMgr.h>
#include <actor/Profile.h>
#include <container/seadSafeArray.h>
#include <effect/EffectCreateUtil.h>
#include <game_info/CourseInfo.h>
#include <graphics/JointBlendModel.h>
#include <graphics/MaterialG3d.h>
#include <graphics/Light.h>
#include <map/CourseData.h>
#include <player/PlayerMgr.h>
#include <player/PlayerObject.h>
#include <player/Yoshi.h>
#include <red/util/SpriteUtil.h>
#include <random/seadGlobalRandom.h>
#include <sound/SndObjectPlayer.h>
#include <ucology/Ucology.h>

const f32 ANIM_BLEND_TIME = 20.0f;

namespace ucology {

/* ===== LUMA ===== */

class Luma : public ActorMultiState {
    SEAD_RTTI_OVERRIDE(Luma, ActorMultiState);
public:
    static Profile* cProfile;
    static const ActorCollisionCheck::CollisionData cCollisionData;
    static const ActorCreateInfo cCreateInfo;

    Luma(const ActorCreateParam& param);
    ~Luma() override = default;

    Result create() override;
    bool execute() override;
    bool draw() override;

    DECLARE_STATE_ID(Luma, Idle);
    DECLARE_STATE_ID(Luma, Bounced);
    DECLARE_STATE_ID(Luma, IdleAnimation);

    void updateModel() const {
        mModel->update(mPos, mAngle, mScale);
    }
    
    void updateLight() {
        f32 lightRadius = 0.5f;
        f32 lightStrength = 0.3f;

        sead::Vector3f lightPos = mPos + mCurrentModelOffset;
        lightPos.y += 10.0f;
        lightPos.z -= 2000.0f;
        mLight.update(static_cast<LightType>(0), &lightPos, nullptr, &lightRadius, &lightStrength, &mColor);
    }

    void setAnimUpdateRate(float rate) const {
        mModel->getCurSklAnim()->getFrameCtrl().setRate(rate);
    }

    void updateCurrentModelOffset() {
        sead::Matrix34f mtx;
        mModel->getModel()->getBoneWorldMatrix(mJointBoneIndex, &mtx);

        mCurrentModelOffset = mtx.getTranslation() - mPos;
    }

    void updateColliderPosition() {
        const f32 CENTER_OFFSET = 14.0f;

        sead::Vector3f offset = mCurrentModelOffset;
        offset.y += CENTER_OFFSET;
        mCollisionCheck.setCenterOffset(offset);
    }
    
    void takePlayerAttention() const {
        AttentionMgr::instance()->entry(mPlayerAttention);
    }

    void updateAttentionPos() {
        sead::Vector2f& pos = mPlayerAttention.getPos();
        pos.x = mPos.x;
        pos.y = mPos.y;
    }

    void releasePlayerAttention() const {
        AttentionMgr::instance()->release(mPlayerAttention);
    }

    void playBounceSFX() {
        mSoundObject.startSound("SE_PLY_CRASH_S", nw::snd::OUTPUT_LINE_MAIN);
    }

    void playBounceGFX() {
        const f32 EFFECT_SCALE = 0.7f;

        sead::Vector3f effectPos = getPos() + mCurrentModelOffset;
        effectPos.y += 6.0f;

        sead::Vector3f effectScale = sead::Vector3f(EFFECT_SCALE, EFFECT_SCALE, EFFECT_SCALE);
        EffectCreateUtil::createPlayerEffect(-1, EffectID_::RP_Npc_Hit, &effectPos, nullptr, &effectScale);
    }

    void setHasLookTarget(bool hasLookTarget) {
        mHasLookTarget = hasLookTarget;
    }

    void setLookTargetPosition(const sead::Vector3f& pos) {
        mLookTargetPosition = pos;
    }

    u8 getLookTargetID() const {
        return mLookTargetID;
    }

    void setModelColor();
    Actor* findClosestStar();
    PlayerObject* findClosestPlayer();
    void faceNearestTarget();

    static void collisionCallback(ActorCollisionCheck* cc_self, ActorCollisionCheck* cc_other);
private:
    // model
    JointBlendModel* mModel;
    sead::Color4f mColor;
    s32 mJointBoneIndex;

    // light
    Light mLight;

    // player is looking at us
    AttentionLookat mPlayerAttention;
    
    // we are looking at things
    f32 mPlayerDistanceThreshold;
    bool mIsFixatedOnSomething;
    ActorUniqueID mLastStarID;
    u32 mStarSearchTimer;
    bool mHasLookTarget;
    sead::Vector3f mLookTargetPosition;
    u8 mLookTargetID;

    // logic
    u32 mBounceTimers[5];
    u32 mRandomIdleAnimTimer;
    bool mFirstTimeInIdleState;
    sead::Vector3f mCurrentModelOffset;

    // sound
    SndObjctPly mSoundObject;
};

SEAD_RTTI_OVERRIDE_IMPL(Luma, ActorMultiState);

using CC = ActorCollisionCheck;
const ActorCollisionCheck::CollisionData Luma::cCollisionData = {
    .center_offset = { 0.0f, 0.0f },
    .half_size = { 6.0f, 2.0f },
    .shape_type = CC::cShapeType_Box,
    .kind = CC::cKind_Enemy,
    .attack = CC::cAttack_None,
    .vs_kind = CC::TargetKind(
        CC::cTargetKind_Player |
        CC::cTargetKind_Yoshi |
        CC::cTargetKind_ChibiYoshi
    ),
    .vs_damage = CC::DamageFrom(CC::cDamageFrom_HipAttack | CC::cDamageFrom_Unk25),
    .status = CC::cStatus_None,
    .callback = Luma::collisionCallback
};

const ActorCreateInfo Luma::cCreateInfo = {
    .offset_x = 8, .offset_y = -8
};

Profile* Luma::cProfile = ucology::getRegistrar()->newProfile<Luma>("luma")
    .resources<"uco_luma">(ProfileInfo::cResType_Course)
    .flag(Profile::cFlag_DrawCullCheck)
    .createInfo(&Luma::cCreateInfo)
    .build();

CREATE_STATE_ID(Luma, Idle);
CREATE_STATE_ID(Luma, Bounced);
CREATE_STATE_ID(Luma, IdleAnimation);

Luma::Luma(const ActorCreateParam& param)
    : ActorMultiState(param)
    , mPlayerAttention(mActorUniqueID)
    , mSoundObject(static_cast<NMSndObject::ObjType>(0), nw::snd::OUTPUT_LINE_MAIN)
{ }

ActorBase::Result Luma::create() {
    const f32 SCALE_FACTOR = 0.15f;
    const f32 DEFAULT_SATURATION = 1.3f;
    const s32 DEFAULT_PLAYER_DISTANCE = 10; // tiles

    // model setup
    mScale = sead::Vector3f(SCALE_FACTOR, SCALE_FACTOR, SCALE_FACTOR);
    mModel = JointBlendModel::create("uco_luma", "uco_luma", 8);
    mJointBoneIndex = mModel->getModel()->searchBoneIndex("AllRoot");
    

    // colors
    bool randomizeColor = static_cast<bool>(red::SpriteUtil::getNybble9(this));
    if (randomizeColor) {
        mColor.r = sead::GlobalRandom::instance()->getF32Range(0.0f, 1.0f);
        mColor.g = sead::GlobalRandom::instance()->getF32Range(0.0f, 1.0f);
        mColor.b = sead::GlobalRandom::instance()->getF32Range(0.0f, 1.0f);
    } else {
        mColor.r = static_cast<f32>(red::SpriteUtil::getNybbleRange(this, 1, 2)) / 255.0f;
        mColor.g = static_cast<f32>(red::SpriteUtil::getNybbleRange(this, 3, 4)) / 255.0f;
        mColor.b = static_cast<f32>(red::SpriteUtil::getNybbleRange(this, 5, 6)) / 255.0f;
    }

    u8 saturation = red::SpriteUtil::getNybble7(this);
    mColor.a = saturation != 0
        ? 0.5f + static_cast<f32>(saturation) * 0.1f
        : DEFAULT_SATURATION;
        
    // other setups
    u8 distance = red::SpriteUtil::getNybble8(this);
    mPlayerDistanceThreshold = 16.0f * (DEFAULT_PLAYER_DISTANCE + distance);
        
    setModelColor();

    updateModel();
    updateLight();
    updateCurrentModelOffset();
    faceNearestTarget();

    // collision setup
    mCollisionCheck.set(this, cCollisionData);
    reviveCollisionCheck();

    // look at
    mLookTargetID = static_cast<u8>(red::SpriteUtil::getNybbleRange(this, 10, 11));
    mHasLookTarget = false;

    // logic
    sead::MemUtil::fill(mBounceTimers, 0, sizeof(mBounceTimers));
    mFirstTimeInIdleState = true;
    mRandomIdleAnimTimer = 0;

    changeState(StateID_Idle);

    return cResult_Success;
}

bool Luma::execute() {
    // model
    updateModel();
    updateLight();
    updateCurrentModelOffset();

    // collision
    updateColliderPosition();

    // logic
    for (u32& timer : mBounceTimers) {
        if (timer > 0) {
            timer--;
        }
    }

    // state
    executeState();

    return true;
}

bool Luma::draw() {
    mModel->draw();
    return true;
}

void Luma::initializeState_Idle() {
    if (mFirstTimeInIdleState) {
        mModel->setAnm("Wait", ANIM_BLEND_TIME, FrameCtrl::cMode_Repeat);
        f32 end = mModel->getCurSklAnim()->getFrameCtrl().getFrameEnd();
        f32 frame = sead::GlobalRandom::instance()->getF32Range(0, end);
        mModel->getCurSklAnim()->getFrameCtrl().setFrame(frame);

        mFirstTimeInIdleState = false;
    } else {
        mModel->setAnm("Wait", ANIM_BLEND_TIME, FrameCtrl::cMode_Repeat);
    }

    f32 rate = sead::GlobalRandom::instance()->getF32Range(0.75f, 1.0f);
    setAnimUpdateRate(rate);
    faceNearestTarget();
}

void Luma::executeState_Idle() {
    const u32 RANDOM_IDLE_TIME_LIMIT_SECONDS = 10;
    const u32 RANDOM_IDLE_TIMER_LIMIT_FRAMES = 60 * RANDOM_IDLE_TIME_LIMIT_SECONDS;

    if (mRandomIdleAnimTimer > 0) {
        mRandomIdleAnimTimer--;
    }

    // 1 in n chance per eligible frame to play an idle animation
    // but don't do this if it's looking at something of interest

    if (!mIsFixatedOnSomething) {
        const u32 RANDOM_IDLE_ANIM_CHANCE = 700;
        if (mRandomIdleAnimTimer == 0) {
            if (sead::GlobalRandom::instance()->getU32(RANDOM_IDLE_ANIM_CHANCE) == 0) {
                changeState(StateID_IdleAnimation);
                mRandomIdleAnimTimer = RANDOM_IDLE_TIMER_LIMIT_FRAMES;
            }
        }
    }

    faceNearestTarget();
}

void Luma::finalizeState_Idle() { }

void Luma::initializeState_Bounced() {
    mModel->setAnm("Trampled", ANIM_BLEND_TIME, FrameCtrl::PlayMode::cMode_NoRepeat);
    setAnimUpdateRate(1.0f);
}

void Luma::executeState_Bounced() {
    if (mModel->getCurSklAnim()->getFrameCtrl().isStop()) {
        changeState(StateID_Idle);
    }
}

void Luma::finalizeState_Bounced() { }

void Luma::initializeState_IdleAnimation() {
    const sead::SafeArray<const char*, 2> ANIM_NAMES = {
        "TouchJoy",
        "Reaction",
    };

    u32 animIndex = sead::GlobalRandom::instance()->getU32(ANIM_NAMES.size());

    mModel->setAnm(ANIM_NAMES[animIndex], ANIM_BLEND_TIME, FrameCtrl::PlayMode::cMode_NoRepeat);
    setAnimUpdateRate(0.5f);
    
    takePlayerAttention();
}

void Luma::executeState_IdleAnimation() {
    updateAttentionPos();

    if (mModel->getCurSklAnim()->getFrameCtrl().isStop()) {
        changeState(StateID_Idle);
    }
}

void Luma::finalizeState_IdleAnimation() {
    releasePlayerAttention();
}

void Luma::setModelColor() {
    for (u32 i = 0; i < mModel->getModel()->getMaterialNum(); i++) {
        MaterialG3d* mat = static_cast<MaterialG3d*>(mModel->getModel()->getMaterial(i));

        s32 paramIndex = mat->getMaterialObj()->GetResource()->GetShaderParamIndex("mat_color0");

        if (paramIndex >= 0) {
            s32 offs = mat->getMaterialObj()->GetResShaderParam(paramIndex)->GetOffset();

            if (offs >= 0) {
                sead::Color4f* col = mat->getMaterialObj()->EditShaderParam<sead::Color4f>(paramIndex);
                col->setf(mColor.r, mColor.g, mColor.b, mColor.a);
            }
        }
    }
}

void Luma::collisionCallback(ActorCollisionCheck* cc_self, ActorCollisionCheck* cc_other) {
    const u32 FRAMES_UNTIL_JUMP_ALLOWED_AGAIN = 10;

    Luma* self = static_cast<Luma*>(cc_self->getOwner());

    Actor* actor = cc_other->getOwner();

    switch (actor->getActorType()) {
        case ActorType::cActorType_Player: {
            PlayerObject* player = static_cast<PlayerObject*>(actor);
            Yoshi* yoshi = player->getRideYoshi();

            if (yoshi != nullptr) {
                // handle yoshi separately
                return;
            }
            
            s8 playerID = player->getPlayerNo();

            if (playerID >= 4) {
                return;
            }

            u32& timer = self->mBounceTimers[playerID];

            if (timer > 0) {
                return;
            }

            timer = FRAMES_UNTIL_JUMP_ALLOWED_AGAIN;

            self->changeState(StateID_Bounced);

            self->playBounceSFX();
            self->playBounceGFX();

            player->bouncePlayer1(4.0f, player->getSpeedF(), true, PlayerBase::cBounceType_Normal, PlayerBase::cJumpSe_Normal);
            break;
        }

        case ActorType::cActorType_Yoshi: {
            Yoshi* yoshi = static_cast<Yoshi*>(actor);
            PlayerObject* player = yoshi->getPlayerRideOn();
            
            if (player != nullptr) {
                s8 playerID = player->getPlayerNo();

                if (playerID >= 4) {
                    return;
                }

                u32& timer = self->mBounceTimers[playerID];

                if (timer > 0) {
                    return;
                }
                
                timer = FRAMES_UNTIL_JUMP_ALLOWED_AGAIN;
                
            } else {
                u32& timer = self->mBounceTimers[4];

                if (timer > 0) {
                    return;
                }

                timer = FRAMES_UNTIL_JUMP_ALLOWED_AGAIN;
            }

            self->playBounceSFX();
            self->changeState(StateID_Bounced);
            self->playBounceGFX();
            yoshi->bouncePlayer1(4.0f, yoshi->getSpeedF(), true, PlayerBase::cBounceType_Normal, PlayerBase::cJumpSe_None);

            break;
        }

        case ActorType::cActorType_ChibiYoshi: {
            // ChibiYoshiBase* baby = static_cast<ChibiYoshiBase*>(actor);
            // todo: figure out how to make it move upward if not being held
            break;
        }
    }
}

Actor* Luma::findClosestStar() {
    // ooh shiny
    constexpr f32 DISTANCE_LIMIT = 16.0f * 20.0f;
    constexpr u32 STAR_SEARCH_TIME = 60 * 3;

    ActorMgr* actorMgr = ActorMgr::instance();

    if (mStarSearchTimer > 0) {
        mStarSearchTimer--;

        // check if the star still exists
        Actor* star = static_cast<Actor*>(actorMgr->getActorPtr(mLastStarID));

        if (star != nullptr) {
            // just keep looking at that one
            return star;
        }

        // otherwise look for a new one
    }
    
    mStarSearchTimer = STAR_SEARCH_TIME;
    
    Actor* star = nullptr;

    f32 distance = DISTANCE_LIMIT;

    for (ActorMgr::iterator it = actorMgr->getActorBegin(); it != actorMgr->getActorEnd(); it++) {
        Actor* actor = static_cast<Actor*>(*it);
        
        if (actor == nullptr || actor->getProfileID() != 853) {
            continue;
        }

        // check if it's close enough
        f32 foundDist = (actor->getPos2D() - getPos2D()).length();
        if (foundDist <= distance) {
            distance = foundDist;
            star = actor;
        }
    }

    return star;
}

PlayerObject* Luma::findClosestPlayer() {
    sead::Vector2f dist;
    s32 playerIndex = searchNearPlayer(dist);

    if (playerIndex == -1 || dist.length() > mPlayerDistanceThreshold) {
        return nullptr;
    }

    return PlayerMgr::instance()->getPlayerObject(playerIndex);
}

void Luma::faceNearestTarget() {
    auto chaseAngle = [](s32 current, s32 target, s32 speed) {
        s32 difference = static_cast<s32>(
            static_cast<u32>(target) -
            static_cast<u32>(current)
        );

        if (difference > speed) {
            return current + speed;
        }

        if (difference < -speed) {
            return current - speed;
        }

        return target;
    };

    constexpr f32 HORIZONTAL_TURN_EXAGGERATION = 5.0f;
    constexpr f32 VERTICAL_TURN_EXAGGERATION = 4.0f;
    constexpr s32 TURN_SPEED = sead::Mathf::deg2idx(3.0f);
    
    sead::Vector3f targetPos;

    // prioritise stars first
    Actor* star = findClosestStar();
    PlayerObject* player = findClosestPlayer();

    mIsFixatedOnSomething = false;
    if (star != nullptr) {
        mIsFixatedOnSomething = true;
        targetPos = star->getPos();
        // use a more suitable z order to make sure the calculations aren't busted
        targetPos.z = 3000.0f;
    } else if (player != nullptr) {
        targetPos = player->getPos();
    } else if (mHasLookTarget) {
        mIsFixatedOnSomething = true;
        targetPos = mLookTargetPosition;
        targetPos.z = 3000.0f;
    } else {
        // look to screen
        mAngle.x() = chaseAngle(mAngle.x(), 0, TURN_SPEED);
        mAngle.y() = chaseAngle(mAngle.y(), 0, TURN_SPEED);
        return;
    }

    sead::Vector3f delta = targetPos - mPos;

    f32 radians = sead::Mathf::atan2(delta.x, delta.z);
    f32 degrees = sead::Mathf::rad2deg(radians);
    degrees *= HORIZONTAL_TURN_EXAGGERATION;

    f32 horizontalDistance = sead::Mathf::sqrt(delta.x * delta.x + delta.z * delta.z);

    f32 pitchRadians = sead::Mathf::atan2(delta.y, horizontalDistance);
    f32 pitchDegrees = sead::Mathf::rad2deg(pitchRadians);
    pitchDegrees *= -VERTICAL_TURN_EXAGGERATION;

    mAngle.x() = chaseAngle(mAngle.x(), sead::Mathf::deg2idx(pitchDegrees), TURN_SPEED);
    mAngle.y() = chaseAngle(mAngle.y(), sead::Mathf::deg2idx(degrees), TURN_SPEED);
}


/* ===== LUMA LOOK LINK TAG ===== */

// TODO: cache actor ids
// TODO: multiple lumas can look at a single object; account for that
class LumaLookTagLink : public Actor {
public:
    static Profile* cProfile;
    static const ActorCreateInfo cCreateInfo;

    LumaLookTagLink(const ActorCreateParam& param);
    ~LumaLookTagLink() override = default;

    Result create() override;
    bool execute() override;

    Luma* findLuma() const;
    Actor* findParentActor() const;
private:
    u8 mLumaID;
    u8 mParentLinkID;
};

const ActorCreateInfo LumaLookTagLink::cCreateInfo = {
    .flag = ActorCreateInfo::cFlag_IgnoreSpawnRange
};

Profile* LumaLookTagLink::cProfile = ucology::getRegistrar()->newProfile<LumaLookTagLink>("luma_look_tag_link")
    .createInfo(&LumaLookTagLink::cCreateInfo)
    .build();

LumaLookTagLink::LumaLookTagLink(const ActorCreateParam& param)
    : Actor(param)
{ }

ActorBase::Result LumaLookTagLink::create() {
    mLumaID = static_cast<u8>(red::SpriteUtil::getNybbleRange(this, 1, 2));
    mParentLinkID = static_cast<u8>(red::SpriteUtil::getNybbleRange(this, 3, 4));

    return cResult_Success;
}

bool LumaLookTagLink::execute() {
    ActorMgr* actorMgr = ActorMgr::instance();

    Luma* luma = findLuma();
    Actor* parent = findParentActor();

    if (luma == nullptr || parent == nullptr) {
        return true;
    }

    if (parent->isRequestedDelete()) {
        luma->setHasLookTarget(false);
        mDeleteRequestFlag = true;
        return true;
    }

    luma->setHasLookTarget(true);

    sead::Vector3f targetPos = mParentLinkID != 0
        ? parent->getPos()
        : getPos();

    luma->setLookTargetPosition(targetPos);

    return true;
}

Luma* LumaLookTagLink::findLuma() const {
    Luma* luma = nullptr;

    ActorMgr* actorMgr = ActorMgr::instance();

    for (ActorMgr::iterator it = actorMgr->getActorBegin(); it != actorMgr->getActorEnd(); it++) {
        Luma* actor = sead::DynamicCast<Luma>(*it);

        if (actor == nullptr) {
            continue;
        }

        u8 linkID = actor->getLookTargetID();

        if (linkID != 0 && linkID == mLumaID) {
            luma = actor;
        }
    }

    return luma;
}

Actor* LumaLookTagLink::findParentActor() const {
    Actor* parent = nullptr;

    ActorMgr* actorMgr = ActorMgr::instance();

    for (ActorMgr::iterator it = actorMgr->getActorBegin(); it != actorMgr->getActorEnd(); it++) {
        Actor* actor = static_cast<Actor*>(*it);

        if (actor == nullptr) {
            continue;
        }

        // use initial state
        u8 linkID = actor->getParamEx().course.init_state_flag;

        if (linkID != 0 && linkID == mParentLinkID) {
            parent = actor;
        }
    }

    return parent;
}

}
