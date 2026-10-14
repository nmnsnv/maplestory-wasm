#include <doctest/doctest.h>
#include "client/Graphics/Shaders.h"

#include <string>

TEST_CASE("fragment shader samples the texture atlas with highp precision")
{
    // Atlas coordinates reach 8192 texels. With mediump (16-bit floats on some
    // GPUs, e.g. Mesa radeonsi) texpos / atlassize is too coarse to address a
    // single texel, so sprites far from the atlas origin render as garbled blocks.
    const std::string source = jrc::Shaders::FRAGMENT;

    CHECK(source.rfind("precision highp float;", 0) == 0);
}
