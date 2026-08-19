// TODO: movement-controlled look target
// TODO: event-controlled look target (look when target is activated, etc.)

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

namespace ucology {

class PlayerLookTag : public Actor {
private:
    enum LookAt : u8 {
        Position,
        PositionWhileInScreen,
        Screen
    };
public:
    static Profile* cProfile;
    static const ActorCreateInfo cCreateInfo;

    PlayerLookTag(const ActorCreateParam& param);
    ~PlayerLookTag() override = default;

    Result create() override;
    bool execute() override;

    void takePlayerAttention() {
        AttentionMgr::instance()->entry(mPlayerAttention);
    }

    void updatePlayerAttention() {
        sead::Vector2f& pos = mPlayerAttention.getPos();
        pos.x = mPos.x;
        pos.y = mPos.y;
    }

    void releasePlayerAttention() {
        AttentionMgr::instance()->release(mPlayerAttention);
    }
private:
    LookAt mPlayerLookType;
    AttentionLookat mPlayerAttention;
    u8 mTargetLocationID;
    bool mHasPlayerAttention;
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

    switch (mPlayerLookType) {
        case LookAt::Position: {
            takePlayerAttention();
            updatePlayerAttention();
            mHasPlayerAttention = true;
            break;
        }

        case LookAt::PositionWhileInScreen:
        case LookAt::Screen: {
            mHasPlayerAttention = false;
            if (mTargetLocationID == 0) {
                tk::fatal("PlayerLookTag - Must set a non-zero location ID to make the player look at the target");
                return cResult_Failed;
            }
        }
    }

    return cResult_Success;
}

bool PlayerLookTag::execute() {
    if (mPlayerLookType == LookAt::Position || mPlayerLookType == LookAt::PositionWhileInScreen) {
        updatePlayerAttention();
        if (mPlayerLookType == LookAt::Position) {
            return true;
        }
    }

    for (s32 i = 0; i < PlayerMgr::instance()->getNum(); i++) {
        PlayerObject* player = PlayerMgr::instance()->getPlayerObject(i);
        
        if (player == nullptr) {
            continue;
        }
        
        // check if player is within a location
        sead::BoundBox2f locationBounds;
        const CourseDataFile* area = CourseData::instance()->getFile(CourseInfo::instance()->getFileNo());
        const Location* location = area->getLocation(&locationBounds, mTargetLocationID);
        if (location == nullptr) {
            tk::fatal("PlayerLookTag - the specified location %d does not exist", mTargetLocationID);
            return false;
        }

        if (locationBounds.isInside(player->getPos2D())) {
            if (mPlayerLookType == LookAt::Screen) {
                // look at the screen
                player->setClampFaceRot();
            } else if (mPlayerLookType == LookAt::PositionWhileInScreen) {
                if (!mHasPlayerAttention) {
                    takePlayerAttention();
                    mHasPlayerAttention = true;
                }
            }
        } else if (mPlayerLookType == LookAt::PositionWhileInScreen && mHasPlayerAttention) {
            releasePlayerAttention();
            mHasPlayerAttention = false;
        }
    }
    
    return true;
}

}