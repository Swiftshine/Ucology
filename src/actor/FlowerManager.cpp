#include <actor/Actor.h>
#include <ucology/Ucology.h>
#include <red/util/SpriteUtil.h>
#include <map/Bg.h>

namespace ucology {

class FlowerManager : public Actor {
public:
    static Profile* cProfile;

    FlowerManager(const ActorCreateParam &param);
    ~FlowerManager() override = default;

    Result create() override;
};

Profile* FlowerManager::cProfile =
    ucology::getRegistrar()
        ->newProfile<FlowerManager>("flower_manager")
        .build();

FlowerManager::FlowerManager(const ActorCreateParam &param)
    : Actor(param)
{ }

ActorBase::Result FlowerManager::create() {
    u8 flowerType = red::SpriteUtil::getNybble1(this) + 1;
    Bg* bg = Bg::instance();
    // todo: modify Bg header
    *(reinterpret_cast<u8*>(bg) + 0x3F8C) = flowerType;
    return cResult_Success;
}

} // namespace ucology
