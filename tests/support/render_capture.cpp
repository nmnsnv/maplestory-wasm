#include "render_capture.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>

namespace test_support
{
    void snapshot(const std::string& path, int width, int height, uint8_t background)
    {
        if (width <= 0 || height <= 0 || width > 4096 || height > 4096)
            throw std::runtime_error("Invalid diagnostic image dimensions");
        std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 3, background);
        for (const auto& draw : draws)
        {
            const auto& rect = draw.bounds;
            const int span_x = std::abs(int(rect.r()) - rect.l());
            const int span_y = std::abs(int(rect.b()) - rect.t());
            if (!span_x || !span_y || !draw.bitmap.width() || !draw.bitmap.height())
                continue;
            const auto* bgra = static_cast<const uint8_t*>(draw.bitmap.data());
            if (!bgra)
                throw std::runtime_error("Cannot decompress diagnostic bitmap");
            for (int y = std::max<int>(0, std::min(rect.t(), rect.b())); y < std::min<int>(height, std::max(rect.t(), rect.b())); ++y)
                for (int x = std::max<int>(0, std::min(rect.l(), rect.r())); x < std::min<int>(width, std::max(rect.l(), rect.r())); ++x)
                {
                    // Mirrored rectangles reverse sampling, not the buffer stride.
                    const auto u = static_cast<size_t>(rect.r() > rect.l() ? x - rect.l() : rect.l() - 1 - x) * draw.bitmap.width() / span_x;
                    const auto v = static_cast<size_t>(rect.b() > rect.t() ? y - rect.t() : rect.t() - 1 - y) * draw.bitmap.height() / span_y;
                    const auto* source = bgra + (v * draw.bitmap.width() + u) * 4;
                    auto* destination = pixels.data() + (static_cast<size_t>(y) * width + x) * 3;
                    const int alpha = static_cast<int>(source[3] * std::clamp(draw.opacity, 0.0f, 1.0f));
                    for (int channel = 0; channel < 3; ++channel)
                        destination[channel] = static_cast<uint8_t>((source[2 - channel] * alpha + destination[channel] * (255 - alpha) + 127) / 255);
                }
        }
        std::ofstream output(path, std::ios::binary);
        output << "P6\n" << width << ' ' << height << "\n255\n";
        output.write(reinterpret_cast<const char*>(pixels.data()), static_cast<std::streamsize>(pixels.size()));
        if (!output)
            throw std::runtime_error("Cannot write diagnostic image: " + path);
    }
}
