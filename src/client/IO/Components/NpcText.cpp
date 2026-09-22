#include "NpcText.h"

#include "../../Data/ItemData.h"
#include "nlnx/nx.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <string_view>

namespace jrc
{
    namespace
    {
        Texture image_for(char token, std::string_view value)
        {
            if (token == 'i' || token == 'v')
            {
                // Some scripts put a colon immediately before the delimiter.
                if (!value.empty() && value.back() == ':') value.remove_suffix(1);
                int32_t id = 0;
                auto number = std::from_chars(value.data(), value.data() + value.size(), id);
                if (number.ec == std::errc{} && number.ptr == value.data() + value.size() && id > 0)
                {
                    const auto& item = ItemData::get(id);
                    if (item.is_valid()) return item.get_icon(false);
                }
                return {};
            }
            const size_t slash = value.find('/');
            if (slash == std::string_view::npos) return {};
            const auto root = value.substr(0, slash);
            // Script paths resolve only within loaded game resources.
            const std::pair<std::string_view, nl::node> roots[] = {
                {"UI", nl::nx::ui}, {"Item", nl::nx::item}, {"Character", nl::nx::character},
                {"Effect", nl::nx::effect}, {"Etc", nl::nx::etc}, {"Map", nl::nx::map},
                {"Mob", nl::nx::mob}, {"Npc", nl::nx::npc}, {"Skill", nl::nx::skill}
            };
            for (const auto& entry : roots)
                if (entry.first == root)
                    return Texture(entry.second.resolve(std::string(value.substr(slash + 1))));
            return {};
        }
    }

    size_t NpcText::image_tag_end(const std::string& source, size_t begin)
    {
        if (begin + 2 >= source.size() || source[begin] != '#') return std::string::npos;
        const char token = source[begin + 1];
        if (token != 'f' && token != 'i' && token != 'v') return std::string::npos;
        const auto end = source.find_first_of("#\r\n", begin + 2);
        return end != std::string::npos && source[end] == '#' ? end + 1 : std::string::npos;
    }

    NpcText::NpcText(const std::string& source, int16_t max_width, Text::Color default_color)
    {
        max_width = std::max<int16_t>(1, max_width);
        Text::Color color = default_color;
        Text::Font font = Text::A12M;
        const int16_t line_space = Text(font, Text::LEFT, color, "Ag").height();
        std::array<std::array<int16_t, 128>, 2> advances;
        for (auto& widths : advances) widths.fill(-1);
        auto advance = [&](char ch) {
            auto& width = advances[font == Text::A12B][static_cast<unsigned char>(ch)];
            if (width < 0)
                width = Text(font, Text::LEFT, color, std::string(1, ch), 0, false).width();
            return width;
        };
        int32_t x = 0, y = 0;
        int16_t line_height = line_space;
        size_t line_begin = 0;
        auto finish_line = [&] {
            for (size_t i = line_begin; i < runs.size(); ++i)
                runs[i].position.set_y(y + (line_height - runs[i].height) / 2);
            content_width = std::max<int16_t>(content_width, static_cast<int16_t>(x));
            y += line_height;
            x = 0;
            line_height = line_space;
            line_begin = runs.size();
        };
        auto add_text = [&](const std::string& word, bool whitespace) {
            int32_t word_width = 0;
            for (char ch : word) word_width += advance(ch);
            if (x > 0 && x + word_width > max_width)
            {
                finish_line();
                if (whitespace) return;
            }
            std::string part;
            auto flush = [&] {
                if (part.empty()) return;
                Text label(font, Text::LEFT, color, part, max_width, false);
                const int16_t label_height = label.height();
                runs.push_back({std::move(label), {}, {x, y}, label_height});
                for (char ch : part) x += advance(ch);
                line_height = std::max(line_height, label_height);
                part.clear();
            };
            int32_t part_width = 0;
            for (char ch : word)
            {
                const int16_t glyph_width = advance(ch);
                if (x + part_width + glyph_width > max_width && (x > 0 || !part.empty()))
                {
                    flush();
                    finish_line();
                    part_width = 0;
                }
                part += ch;
                part_width += glyph_width;
            }
            flush();
        };

        for (size_t cursor = 0; cursor < source.size();)
        {
            const char ch = source[cursor];
            if (ch == '\r' || ch == '\n' ||
                (ch == '\\' && cursor + 1 < source.size() && (source[cursor + 1] == 'r' || source[cursor + 1] == 'n')))
            {
                if (ch == '\\')
                {
                    const bool cr = source[cursor + 1] == 'r';
                    cursor += 2;
                    if (cr && source.compare(cursor, 2, "\\n") == 0) cursor += 2;
                }
                else
                {
                    ++cursor;
                    if (ch == '\r' && cursor < source.size() && source[cursor] == '\n') ++cursor;
                }
                finish_line();
                continue;
            }
            if (ch == '#' && cursor + 1 < source.size())
            {
                const char token = source[cursor + 1];
                const size_t image_end = image_tag_end(source, cursor);
                if (image_end != std::string::npos)
                {
                    Texture image = image_for(token, std::string_view(source).substr(cursor + 2, image_end - cursor - 3));
                    if (image.is_valid() && image.width() > 0 && image.height() > 0)
                    {
                        if (x > 0 && x + image.width() > max_width) finish_line();
                        const int16_t image_width = std::min(max_width, image.width());
                        const int16_t image_height = std::max<int16_t>(1, static_cast<int16_t>(
                            static_cast<int32_t>(image.height()) * image_width / image.width()));
                        runs.push_back({{}, std::move(image), {x, y}, image_height});
                        x += image_width;
                        line_height = std::max(line_height, image_height);
                    }
                    cursor = image_end;
                    continue;
                }
                cursor += 2;
                switch (token)
                {
                case '#': add_text("#", false); break;
                case 'b': color = Text::BLUE; break;
                case 'r': color = Text::RED; break;
                case 'k': color = default_color; break;
                case 'd': color = Text::VIOLET; break;
                case 'e': font = Text::A12B; break;
                case 'n': font = Text::A12M; break;
                case 'f': case 'i': case 'v':
                    // Malformed image tags must not expose resource paths.
                    cursor = source.find_first_of("\r\n", cursor);
                    if (cursor == std::string::npos) cursor = source.size();
                    break;
                default:
                {
                    size_t end = cursor;
                    while (end < source.size() && std::isdigit(static_cast<unsigned char>(source[end]))) ++end;
                    if (end > cursor && end < source.size() && source[end] == '#') cursor = end + 1;
                    break;
                }
                }
                continue;
            }
            const bool whitespace = ch == ' ' || ch == '\t';
            std::string word;
            while (cursor < source.size())
            {
                const unsigned char next = static_cast<unsigned char>(source[cursor]);
                if (next == '#' || next == '\r' || next == '\n' || next == '\\' ||
                    (next == ' ' || next == '\t') != whitespace) break;
                // The font atlas is ASCII: do not index it with UTF-8 bytes.
                if (next >= 128)
                {
                    if ((next & 0xC0) != 0x80) word += '?';
                }
                else if (next == '\t') word += "    ";
                else if (next >= 32) word += static_cast<char>(next);
                ++cursor;
            }
            if (word.empty() && cursor < source.size()) word += source[cursor++];
            add_text(word, whitespace);
        }
        if (line_begin < runs.size() || y > 0) finish_line();
        content_height = y;
    }

    void NpcText::change_color(Text::Color color)
    {
        for (auto& run : runs) run.text.change_color(color);
    }

    void NpcText::draw_clipped(Point<int32_t> position, Range<int16_t> vertical) const
    {
        for (const auto& run : runs)
        {
            const auto point = position + run.position;
            if (point.y() + run.height <= vertical.first() || point.y() >= vertical.second()) continue;
            const Point<int16_t> drawpoint(static_cast<int16_t>(point.x()), static_cast<int16_t>(point.y()));
            if (run.image.is_valid())
            {
                const auto origin = run.image.get_origin();
                const float scale = static_cast<float>(run.height) / run.image.height();
                const Point<int16_t> anchor(drawpoint.x() + static_cast<int16_t>(origin.x() * scale),
                    drawpoint.y() + static_cast<int16_t>(origin.y() * scale));
                run.image.draw_clipped(DrawArgument(anchor, scale, scale), vertical);
            }
            else run.text.draw_clipped(drawpoint, vertical);
        }
    }
}
