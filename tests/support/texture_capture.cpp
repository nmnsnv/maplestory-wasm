#include "render_capture.h"
#include "client/Graphics/Texture.h"

namespace jrc
{
    // These composition tests observe draw placement without a graphics atlas.
    // Separate button tests link Texture.cpp to cover production source lookup.
    Texture::Texture() {}
    Texture::Texture(nl::node source)
    {
        if (source.data_type() != nl::node::type::bitmap)
            return;
        const std::string link = source["source"];
        if (!link.empty())
            source = source.root().resolve(link.substr(link.find('/') + 1));
        bitmap = source.get_bitmap();
        origin = source["origin"];
        dimensions = {static_cast<int16_t>(bitmap.width()), static_cast<int16_t>(bitmap.height())};
    }
    Texture::~Texture() {}
    void Texture::draw(const DrawArgument& args) const
    {
        if (bitmap.id())
            test_support::draws.push_back({bitmap, args.get_rectangle(origin, dimensions), args.get_color().a()});
    }
    void Texture::shift(Point<int16_t> amount) { origin -= amount; }
    bool Texture::is_valid() const { return bitmap.id() > 0; }
    int16_t Texture::width() const { return dimensions.x(); }
    int16_t Texture::height() const { return dimensions.y(); }
    Point<int16_t> Texture::get_origin() const { return origin; }
    Point<int16_t> Texture::get_dimensions() const { return dimensions; }
}
