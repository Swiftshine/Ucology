#include <actor/Actor.h>
#include <ucology/actor/PlayerLookTag.h>
#include <red/util/SpriteUtil.h>
#include <actor/ActorMgr.h>

namespace ucology {

class PlayerLookTagLink : public Actor {
public:
    static Profile* cProfile;
    
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

Profile* PlayerLookTagLink::cProfile = ucology::getRegistrar()->newProfile<PlayerLookTagLink>("player_look_tag_link").build();

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
        Actor* actor = sead::DynamicCast<Actor>(*it);

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