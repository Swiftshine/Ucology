#include <actor/Actor.h>
#include <actor/ActorMgr.h>
#include <ucology/Ucology.h>
#include <map/Bg.h>
#include <red/util/SpriteUtil.h>

namespace ucology {

class Flower : public Actor {
public:
    static Profile* cProfile;
    Flower(const ActorCreateParam& param);
    ~Flower() override = default;

    Result create() override;
};

Profile* Flower::cProfile = ucology::getRegistrar()->newProfile<Flower>("flower").build();

Flower::Flower(const ActorCreateParam &param) : Actor(param) {}

ActorBase::Result Flower::create() {
    // Bg::instance()->registerFlower(mPos.x, mPos.y, mPos.z, 2, 0, 0x0);

    u8 color = 0x8 + red::SpriteUtil::getNybble1(this);

    // spawn flower actor
    ActorCreateParam param;
    param.param_0 = 0x1000020 | color;
    param.profile = Profile::get(745);
    param.position = mPos;
    param.param_ex_0.course.layer = 0; // layer 1
    ActorMgr::instance()->createImmediately(param);

    return cResult_Success;
}


}
