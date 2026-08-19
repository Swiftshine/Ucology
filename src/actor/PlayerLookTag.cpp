
#include <ucology/actor/PlayerLookTag.h>

#include <player/PlayerMgr.h>
#include <player/PlayerObject.h>
#include <red/util/SpriteUtil.h>

SEAD_RTTI_OVERRIDE_IMPL(ucology::PlayerLookTag, Actor);

namespace ucology {

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
    mLinkID = static_cast<u8>(red::SpriteUtil::getNybbleRange(this, 6, 7));
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
        takePlayerAttentionIfNotTaken();
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
                takePlayerAttentionIfNotTaken();
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