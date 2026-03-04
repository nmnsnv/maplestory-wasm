#pragma once
#include "DrawArgument.h"

#include "../Template/Point.h"

#include <cstdint>

namespace jrc
{
    class Geometry
    {
    public:
        static const size_t NUM_COLORS = 7;
        enum Color
        {
            // Common
            BLACK,
            WHITE,

            // Mob hp bar
            HPBAR_LIGHTGREEN,
            HPBAR_GREEN,
            HPBAR_DARKGREEN,
            HPBAR_LIGHTRED,
            HPBAR_DARKRED
        };

        virtual ~Geometry() {}

    protected:
        void draw(int16_t x, int16_t y, int16_t w, int16_t h, Geometry::Color color, float opacity) const;
    };


    class ColorBox : public Geometry
    {
    public:
        ColorBox(int16_t width, int16_t height, Geometry::Color color, float opacity);
        ColorBox();

        void setwidth(int16_t width);
        void setheight(int16_t height);
        void set_color(Geometry::Color color);
        void setopacity(float opacity);

        void draw(const DrawArgument& args) const;

    private:
        int16_t width;
        int16_t height;
        Geometry::Color color;
        float opacity;
    };


    class ColorLine : public Geometry
    {
    public:
        ColorLine(int16_t width, Geometry::Color color, float opacity);
        ColorLine();

        void setwidth(int16_t width);
        void set_color(Geometry::Color color);
        void setopacity(float opacity);

        void draw(const DrawArgument& args) const;

    private:
        int16_t width;
        Geometry::Color color;
        float opacity;
    };


    class MobHpBar : public Geometry
    {
    public:
        void draw(Point<int16_t> position, int16_t hppercent) const;

    private:
        static const int16_t WIDTH = 50;
        static const int16_t HEIGHT = 10;
    };

    class PartyHpBar : public Geometry
    {
    public:
        void draw(Point<int16_t> position, int16_t hppercent) const;

    private:
        static const int16_t WIDTH = 50;
        static const int16_t HEIGHT = 10;
    };
}
