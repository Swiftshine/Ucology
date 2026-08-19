#include <actor/Actor.h>
#include <actor/Profile.h>
#include <actor/AttentionLookat.h>
#include <actor/AttentionMgr.h>
#include <game_info/CourseInfo.h>
#include <map/CourseData.h>
#include <ucology/Ucology.h>
#include <player/PlayerMgr.h>
#include <player/PlayerObject.h>
#include <red/util/SpriteUtil.h>
#include <map/SwitchFlagMgr.h>

namespace ucology {

class PlayerLookTag : public Actor {
private:
    enum LookAt : u8 {
        PositionAlways,
        PositionWhileInLocation,
        ScreenWhileInLocation
    };
public:
    static Profile* cProfile;
    static const ActorCreateInfo cCreateInfo;

    PlayerLookTag(const ActorCreateParam& param);
    ~PlayerLookTag() override = default;

    Result create() override;
    bool execute() override;

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

    void releasePlayerAttentionIfTaken() {
        if (mHasPlayerAttention) {
            releasePlayerAttention();
            mHasPlayerAttention = false;
        }
    }

    bool isEventActive() const {
        return mEventID != 0 ? SwitchFlagMgr::instance()->isActivated(mEventID - 1) : true;
    }

    sead::BoundBox2f getLocationBounds() const {
        sead::BoundBox2f bounds;
        const CourseDataFile* area = CourseData::instance()->getFile(CourseInfo::instance()->getFileNo());
        const Location* location = area->getLocation(&bounds, mTargetLocationID);

        if (location == nullptr) {
            tk::fatal("PlayerLookTag - the specified location (id %d) does not exist", mTargetLocationID);
            return bounds;
        }

        return bounds;
    }

    void update_LookAt_PositionAlways();
    void update_LookAt_PositionWhileInLocation();
    void update_LookAt_ScreenWhileInLocation();
private:
    LookAt mPlayerLookType;
    AttentionLookat mPlayerAttention;
    u8 mTargetLocationID;
    bool mHasPlayerAttention;
    u8 mEventID;
};

const ActorCreateInfo PlayerLookTag::cCreateInfo = {
    .offset_x = 8, .offset_y = -8
};

Profile* PlayerLookTag::cProfile = ucology::getRegistrar()->newProfile<PlayerLookTag>("player_look_tag")
    .createInfo(&cCreateInfo)
    .build();

PlayerLookTag::PlayerLookTag(const ActorCreateParam& param)
    : Actor(param)
    , mPlayerAttention(mActorUniqueID)
{ }

ActorBase::Result PlayerLookTag::create() {
    mPlayerLookType = static_cast<LookAt>(red::SpriteUtil::getNybble1(this));
    mTargetLocationID = static_cast<u8>(red::SpriteUtil::getNybbleRange(this, 2, 3));
    mEventID = static_cast<u8>(red::SpriteUtil::getNybbleRange(this, 4, 5));
    mHasPlayerAttention = false;

    return cResult_Success;
}

bool PlayerLookTag::execute() {
    switch (mPlayerLookType) {
        case LookAt::PositionAlways: update_LookAt_PositionAlways(); break;
        case LookAt::PositionWhileInLocation: update_LookAt_PositionWhileInLocation(); break;
        case LookAt::ScreenWhileInLocation: update_LookAt_ScreenWhileInLocation(); break;
    }
    return true;
}

void PlayerLookTag::update_LookAt_PositionAlways() {
    updateAttentionPos();

    if (isEventActive()) {
        if (!mHasPlayerAttention) {
            takePlayerAttention();
            mHasPlayerAttention = true;
        }
    } else {
        releasePlayerAttentionIfTaken();
    }
}

void PlayerLookTag::update_LookAt_PositionWhileInLocation() {
    updateAttentionPos();

    if (isEventActive()) {
        for (s32 i = 0; i < PlayerMgr::instance()->getNum(); i++) {
            PlayerObject* player = PlayerMgr::instance()->getPlayerObject(i);

            if (player == nullptr) {
                continue;
            }

            // check if player is within bounds
            if (getLocationBounds().isInside(player->getPos2D())) {
                if (!mHasPlayerAttention) {
                    takePlayerAttention();
                    mHasPlayerAttention = true;   
                }
            } else {
                releasePlayerAttentionIfTaken();
            }
        }
    } else {
        releasePlayerAttentionIfTaken();
    }
}

void PlayerLookTag::update_LookAt_ScreenWhileInLocation() {
    if (isEventActive()) {
        for (s32 i = 0; i < PlayerMgr::instance()->getNum(); i++) {
            PlayerObject* player = PlayerMgr::instance()->getPlayerObject(i);

            if (player == nullptr) {
                continue;
            }

            // check if player is within bounds
            if (getLocationBounds().isInside(player->getPos2D())) {
                // look at the screen
                player->setClampFaceRot();
            }
        }
    }
}

}