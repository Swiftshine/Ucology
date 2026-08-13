#include <actor/Actor.h>
#include <ucology/Ucology.h>
#include <map/Bg.h>
#include <red/util/SpriteUtil.h>

namespace ucology {

class Butterfly : public Actor {
public:
    static Profile* cProfile;

    Butterfly(const ActorCreateParam& param);
    ~Butterfly() override = default;

    Result create() override;
};


Profile* Butterfly::cProfile = ucology::getRegistrar()->newProfile<Butterfly>("butterfly").build();

Butterfly::Butterfly(const ActorCreateParam &param) : Actor(param) {}

ActorBase::Result Butterfly::create() {
    Bg::instance()->registerButterfly(mPos.x, mPos.y, mPos.z, red::SpriteUtil::getNybble1(this), 0xFF, 0);
    return cResult_Success;
}


}
