#pragma once

#include <actor/Actor.h>
#include <actor/Profile.h>
#include <actor/AttentionLookat.h>
#include <actor/AttentionMgr.h>
#include <game_info/CourseInfo.h>
#include <map/CourseData.h>
#include <map/SwitchFlagMgr.h>
#include <ucology/Ucology.h>

namespace ucology {

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

}