#include "MapEffect.h"

#include "../../Console.h"
#include "../../Constants.h"

#include "nlnx/nx.hpp"

namespace jrc
{
    MapEffect::MapEffect(const std::string& path)
        : finished(false),
          position(Constants::viewwidth() / 2, 250)
    {
        nl::node effect_node = nl::nx::map["Effect.img"].resolve(path);
        if (!effect_node)
        {
            Console::get().print("[MapEffect] Missing NX effect path: " + path);
            finished = true;
            return;
        }

        effect = Animation(effect_node);
    }

    MapEffect::MapEffect()
        : finished(true)
    {}

    void MapEffect::draw() const
    {
        if (!finished)
        {
            effect.draw(DrawArgument(position), 1.0f);
        }
    }

    void MapEffect::update()
    {
        if (!finished)
        {
            finished = effect.update(6);
        }
    }
}
