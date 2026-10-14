#pragma once

namespace jrc
{
    namespace Shaders
    {
        constexpr const char* VERTEX =
            "precision highp float;\n"
            "attribute vec4 coord;"
            "attribute vec4 color;"
            "varying vec2 texpos;"
            "varying vec4 colormod;"
            "uniform vec2 screensize;"
            "uniform int yoffset;"

            "void main(void) {"
            "    float x = -1.0 + coord.x * 2.0 / screensize.x;"
            "    float y = 1.0 - (coord.y + float(yoffset)) * 2.0 / screensize.y;"
            "    gl_Position = vec4(x, y, 0.0, 1.0);"
            "    texpos = coord.zw;"
            "    colormod = color;"
            "}";

        // highp: texpos / atlassize must resolve single texels of the 8192px
        // atlas. mediump is 16-bit on some GPUs (e.g. Mesa radeonsi) and
        // garbles sprites far from the atlas origin.
        constexpr const char* FRAGMENT =
            "precision highp float;\n"
            "varying vec2 texpos;"
            "varying vec4 colormod;"
            "uniform sampler2D texture;"
            "uniform vec2 atlassize;"
            "uniform int fontregion;"

            "void main(void) {"
            "    if (texpos.y == 0.0) {"
            "        gl_FragColor = colormod;"
            "    } else if (texpos.y <= float(fontregion)) {"
            "        gl_FragColor = vec4(1.0, 1.0, 1.0, texture2D(texture, texpos / atlassize).r) * colormod;"
            "    } else {"
            "        gl_FragColor = texture2D(texture, texpos / atlassize) * colormod;"
            "    }"
            "}";
    }
}
