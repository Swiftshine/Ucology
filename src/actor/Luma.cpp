// TODO: spin when:
// - touching a player with star power
// - touching a baby yoshi with star power
// - being touched by yoshi's tongue

#include <array>

#include <actor/ActorState.h>
#include <actor/AttentionMgr.h>
#include <actor/Profile.h>
#include <effect/EffectCreateUtil.h>
#include <graphics/JointBlendModel.h>
#include <graphics/MaterialG3d.h>
#include <graphics/Light.h>
#include <player/PlayerMgr.h>
#include <player/PlayerObject.h>
#include <player/Yoshi.h>
#include <red/util/SpriteUtil.h>
#include <random/seadGlobalRandom.h>
#include <sound/SndObjectPlayer.h>
#include <ucology/Ucology.h>

#include <imgui/imgui.h>

const f32 ANIM_BLEND_TIME = 20.0f;

namespace ucology {

class Luma : public ActorMultiState {
public:
    static Profile* cProfile;
    static const ActorCollisionCheck::CollisionData cCollisionData;
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

    void setModelColor();
    void faceNearestPlayer();

    void debugMenu();
    static void collisionCallback(ActorCollisionCheck* cc_self, ActorCollisionCheck* cc_other);
private:
    // model
    JointBlendModel* mModel;
    sead::Color4f mColor;
    s32 mJointBoneIndex;

    // light
    Light mLight;

    // player attention
    AttentionLookat mPlayerAttention;

    // logic
    u32 mBounceTimers[4];
    u32 mRandomIdleAnimTimer;
    bool mFirstTimeInIdleState;
    sead::Vector3f mCurrentModelOffset;

    // sound
    SndObjctPly mSoundObject;
};

using CC = ActorCollisionCheck;
const ActorCollisionCheck::CollisionData Luma::cCollisionData = {
    .center_offset = { 0.0f, 0.0f },
    .half_size = { 4.0f, 2.0f },
    .shape_type = CC::cShapeType_Box,
    .kind = CC::cKind_Enemy,
    .attack = CC::cAttack_None,
    .vs_kind = CC::TargetKind(
        CC::cTargetKind_Player |
        CC::cTargetKind_Yoshi
    ),
    .vs_damage = CC::DamageFrom(CC::cDamageFrom_HipAttack | CC::cDamageFrom_Unk25),
    .status = CC::cStatus_None,
    .callback = Luma::collisionCallback
};

Profile* Luma::cProfile = ucology::getRegistrar()->newProfile<Luma>("luma")
    .resources<"uco_luma">(ProfileInfo::cResType_Course)
    .flag(Profile::cFlag_DrawCullCheck)
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

    // model setup
    mScale = sead::Vector3f(SCALE_FACTOR, SCALE_FACTOR, SCALE_FACTOR);

    mColor.r = static_cast<f32>(red::SpriteUtil::getNybbleRange(this, 1, 2)) / 255.0f;
    mColor.g = static_cast<f32>(red::SpriteUtil::getNybbleRange(this, 3, 4)) / 255.0f;
    mColor.b = static_cast<f32>(red::SpriteUtil::getNybbleRange(this, 5, 6)) / 255.0f;
    
    u8 saturation = red::SpriteUtil::getNybble7(this);

    if (saturation == 0) {
        mColor.a = DEFAULT_SATURATION;
    } else {
        mColor.a = 0.5f + static_cast<f32>(saturation) * 0.1f;
    }

    mModel = JointBlendModel::create("uco_luma", "uco_luma", 8);
    mJointBoneIndex = mModel->getModel()->searchBoneIndex("AllRoot");

    setModelColor();

    updateModel();
    updateLight();
    updateCurrentModelOffset();

    // collision setup
    mCollisionCheck.set(this, cCollisionData);
    reviveCollisionCheck();

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

    debugMenu();

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
}

void Luma::executeState_Idle() {
    const u32 RANDOM_IDLE_TIME_LIMIT_SECONDS = 10;
    const u32 RANDOM_IDLE_TIMER_LIMIT_FRAMES = 60 * RANDOM_IDLE_TIME_LIMIT_SECONDS;

    if (mRandomIdleAnimTimer > 0) {
        mRandomIdleAnimTimer--;
    }

    // 1 in n chance per eligible frame to play an idle animation
    const u32 RANDOM_IDLE_ANIM_CHANCE = 700;
    if (mRandomIdleAnimTimer == 0) {
        if (sead::GlobalRandom::instance()->getU32(RANDOM_IDLE_ANIM_CHANCE) == 0) {
            changeState(StateID_IdleAnimation);
            mRandomIdleAnimTimer = RANDOM_IDLE_TIMER_LIMIT_FRAMES;
        }
    }

    faceNearestPlayer();
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
    const std::array<const char*, 2> ANIM_NAMES = {
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
    const f32 EFFECT_SCALE = 0.7f;

    Luma* self = static_cast<Luma*>(cc_self->getOwner());

    Actor* actor = cc_other->getOwner();

    switch (actor->getActorType()) {
        case ActorType::cActorType_Player: {
            PlayerObject* player = static_cast<PlayerObject*>(actor);

            s8 playerID = player->getPlayerNo();

            if (playerID >= 4) {
                return;
            }

            u32& timer = self->mBounceTimers[playerID];

            if (timer > 0) {
                return;
            }

            timer = FRAMES_UNTIL_JUMP_ALLOWED_AGAIN;

            self->mSoundObject.startSound("SE_PLY_CRASH_S", nw::snd::OUTPUT_LINE_MAIN);

            sead::Vector3f effectPos = self->getPos() + self->mCurrentModelOffset;
            effectPos.y += 6.0f;

            sead::Vector3f effectScale = sead::Vector3f(EFFECT_SCALE, EFFECT_SCALE, EFFECT_SCALE);
            EffectCreateUtil::createPlayerEffect(-1, EffectID_::RP_Npc_Hit, &effectPos, nullptr, &effectScale);

            player->bouncePlayer1(4.0f, player->getSpeedF(), true, PlayerBase::cBounceType_Normal, PlayerBase::cJumpSe_Normal);
            self->changeState(StateID_Bounced);
            break;
        }

        case ActorType::cActorType_Yoshi: {
            Yoshi* yoshi = static_cast<Yoshi*>(actor);

            
            // todo
            break;
        }

        case ActorType::cActorType_ChibiYoshi: {
            // todo
            break;
        }
    }
}

f32 bam32_to_degrees(s32 bam) {
    constexpr float BAM32_TO_DEG = 360.0f / 4294967296.0f; 
    return static_cast<float>(bam) * BAM32_TO_DEG;
}

s32 degrees_to_bam32(f32 deg) {
    constexpr float DEG_TO_BAM32 = 4294967296.0f / 360.0f; 
    float scaled = deg * DEG_TO_BAM32;

    if (scaled >= 0.0f) {
        scaled = scaled + 0.5f;
    } else {
        scaled = scaled - 0.5f;
    }

    return (s32)scaled;

}

s32 chaseAngle(s32 current, s32 target, s32 speed) {
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
}

void Luma::faceNearestPlayer() {
    const f32 PLAYER_DISTANCE_THRESHOLD = 16.0f * 8.0f;
    const f32 LATERAL_TURN_EXAGGERATION = 5.0f;
    const f32 VERTICAL_TURN_EXAGGERATION = 6.0f;
    const s32 TURN_SPEED = degrees_to_bam32(3.0f);

    sead::Vector2f dist;
    s32 playerIndex = searchNearPlayer(dist);

    if (playerIndex == -1 || dist.length() > PLAYER_DISTANCE_THRESHOLD) {
        mAngle.x() = chaseAngle(mAngle.x(), 0, TURN_SPEED);
        mAngle.y() = chaseAngle(mAngle.y(), 0, TURN_SPEED);
        return;
    }

    PlayerObject* player = PlayerMgr::instance()->getPlayerObject(playerIndex);

    if (player == nullptr) {
        return;
    }

    sead::Vector3f delta = player->getPos() - mPos;

    f32 radians = sead::Mathf::atan2(delta.x, delta.z);
    f32 degrees = sead::Mathf::rad2deg(radians);
    degrees *= LATERAL_TURN_EXAGGERATION;

    f32 horizontalDistance = sead::Mathf::sqrt(
        delta.x * delta.x + delta.z * delta.z
    );

    f32 pitchRadians = sead::Mathf::atan2(delta.y, horizontalDistance);
    f32 pitchDegrees = sead::Mathf::rad2deg(pitchRadians);
    pitchDegrees *= -VERTICAL_TURN_EXAGGERATION;

    mAngle.x() = chaseAngle(
        mAngle.x(),
        degrees_to_bam32(pitchDegrees),
        TURN_SPEED
    );

    mAngle.y() = chaseAngle(
        mAngle.y(),
        degrees_to_bam32(degrees),
        TURN_SPEED
    );
}


void Luma::debugMenu() {
    // if (ImGui::Begin("Angle")) {
    //     sead::Vector3f angles(
    //         bam32_to_degrees(mAngle.x()),
    //         bam32_to_degrees(mAngle.y()),
    //         bam32_to_degrees(mAngle.z())
    //     );

    //     ImGui::DragFloat3("Angles", (float*)&angles);

    //     mAngle.x() = degrees_to_bam32(angles.x);
    //     mAngle.y() = degrees_to_bam32(angles.y);
    //     mAngle.z() = degrees_to_bam32(angles.z);
    // }

    // ImGui::End();
}

}