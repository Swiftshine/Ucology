
#include <actor/Actor.h>
#include <actor/Profile.h>
#include <actor/AttentionLookat.h>
#include <actor/AttentionMgr.h>
#include <game_info/CourseInfo.h>
#include <map/CourseData.h>
#include <map/SwitchFlagMgr.h>
#include <ucology/Ucology.h>
#include <player/PlayerMgr.h>
#include <player/PlayerObject.h>
#include <red/util/SpriteUtil.h>
#include <actor/ActorMgr.h>

namespace ucology {

/* ===== PLAYER LOOK TAG ===== */

class PlayerLookTag : public Actor {
    SEAD_RTTI_OVERRIDE(PlayerLookTag, Actor);
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

    void takePlayerAttentionIfNotTaken() {
        if (!mHasPlayerAttention) {
            takePlayerAttention();
            mHasPlayerAttention = true;
        }
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
    
    u8 getLinkID() const {
        return mLinkID;
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
    u8 mLinkID;
};

SEAD_RTTI_OVERRIDE_IMPL(ucology::PlayerLookTag, Actor);

const ActorCreateInfo PlayerLookTag::cCreateInfo = {
    .offset_x = 8, .offset_y = -8,
    .flag = ActorCreateInfo::cFlag_IgnoreSpawnRange
};

Profile* PlayerLookTag::cProfile = ucology::getRegistrar()->newProfile<PlayerLookTag>("player_look_tag")
    .createInfo(&PlayerLookTag::cCreateInfo)
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

/* ===== PLAYER LOOK TAG LINK ===== */

class PlayerLookTagLink : public Actor {
public:
    static Profile* cProfile;
    static const ActorCreateInfo cCreateInfo;

    PlayerLookTagLink(const ActorCreateParam& param);
    ~PlayerLookTagLink() override = default;

    Result create() override;
    bool execute() override;

    PlayerLookTag* findLookTag() const;
    Actor* findParentActor() const;
private:
    u8 mParentLinkID;
    u8 mLookTagID;
};

const ActorCreateInfo PlayerLookTagLink::cCreateInfo = {
    .flag = ActorCreateInfo::cFlag_IgnoreSpawnRange
};

Profile* PlayerLookTagLink::cProfile = ucology::getRegistrar()->newProfile<PlayerLookTagLink>("player_look_tag_link")
    .createInfo(&PlayerLookTagLink::cCreateInfo)    
    .build();

PlayerLookTagLink::PlayerLookTagLink(const ActorCreateParam& param)
    : Actor(param)
{ }

ActorBase::Result PlayerLookTagLink::create() {
    mParentLinkID = static_cast<u8>(red::SpriteUtil::getNybbleRange(this, 1, 2));
    mLookTagID = static_cast<u8>(red::SpriteUtil::getNybbleRange(this, 3, 4));

    return cResult_Success;
}

bool PlayerLookTagLink::execute() {
    ActorMgr* actorMgr = ActorMgr::instance();

    PlayerLookTag* tag = findLookTag();
    Actor* parent = findParentActor();

    if (tag == nullptr || parent == nullptr) {
        return true;
    }

    if (parent->isRequestedDelete()) {
        delete tag;
        mDeleteRequestFlag = true;
    }

    tag->getPos() = parent->getCenterPos();

    return true;
}

PlayerLookTag* PlayerLookTagLink::findLookTag() const {
    PlayerLookTag* tag = nullptr;

    ActorMgr* actorMgr = ActorMgr::instance();

    for (ActorMgr::iterator it = actorMgr->getActorBegin(); it != actorMgr->getActorEnd(); it++) {
        PlayerLookTag* actor = sead::DynamicCast<PlayerLookTag>(*it);

        if (actor == nullptr) {
            continue;
        }

        u8 linkID = actor->getLinkID();

        if (linkID != 0 && linkID == mLookTagID) {
            tag = actor;
        }
    }

    return tag;
}

Actor* PlayerLookTagLink::findParentActor() const {
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
