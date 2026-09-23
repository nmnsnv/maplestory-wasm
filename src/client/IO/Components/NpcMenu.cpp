#include "NpcMenu.h"

#include <charconv>
#include <cctype>
#include <optional>

namespace jrc
{
    namespace
    {
        bool formatting(char token)
        {
            switch (token)
            {
            case 'b': case 'd': case 'e': case 'g': case 'k': case 'n': case 'r': return true;
            default: return false;
            }
        }

        std::string normalize_newlines(const std::string& source)
        {
            std::string result;
            for (size_t i = 0; i < source.size(); ++i)
            {
                if (source[i] == '\\' && i + 1 < source.size() &&
                    (source[i + 1] == 'r' || source[i + 1] == 'n'))
                {
                    const bool cr = source[++i] == 'r';
                    if (cr && source.compare(i + 1, 2, "\\n") == 0) i += 2;
                    result += '\n';
                }
                else if (source[i] == '\r')
                {
                    if (i + 1 < source.size() && source[i + 1] == '\n') ++i;
                    result += '\n';
                }
                else result += source[i];
            }
            return result;
        }

        std::string trim_end(std::string text)
        {
            while (!text.empty())
            {
                if (std::isspace(static_cast<unsigned char>(text.back()))) text.pop_back();
                else if (text.size() >= 2 && text[text.size() - 2] == '#' && formatting(text.back()))
                    text.resize(text.size() - 2);
                else break;
            }
            return text;
        }

        struct Tag { size_t begin; size_t end; int32_t id; };
        std::optional<Tag> next_tag(const std::string& source, size_t start)
        {
            for (size_t begin = source.find("#L", start); begin != std::string::npos;
                begin = source.find("#L", begin + 2))
            {
                const size_t digits = begin + 2;
                size_t end = digits;
                while (end < source.size() && std::isdigit(static_cast<unsigned char>(source[end]))) ++end;
                if (end == digits || end == source.size() || source[end] != '#') continue;
                int32_t id = 0;
                const auto number = std::from_chars(source.data() + digits, source.data() + end, id);
                if (number.ec == std::errc{}) return Tag{begin, end + 1, id};
            }
            return std::nullopt;
        }
    }

    NpcMenu NpcMenu::parse(const std::string& source)
    {
        // Local WZ dialogue uses literal escape sequences; server scripts use
        // actual newlines. Normalize before separating labels from menu spacing.
        const std::string text = normalize_newlines(source);
        NpcMenu menu;
        size_t cursor = 0;
        auto tag = next_tag(text, cursor);
        while (tag)
        {
            const std::string prose = trim_end(text.substr(cursor, tag->begin - cursor));
            if (!prose.empty()) menu.prompt += prose;
            const auto next = next_tag(text, tag->end);
            size_t end = text.find("#l", tag->end);
            // A missing closing marker must not swallow the following answer,
            // even when that later answer does have a closing marker.
            const bool closed = end != std::string::npos && (!next || end < next->begin);
            if (!closed) end = next ? next->begin : text.size();
            menu.options.push_back({tag->id, trim_end(text.substr(tag->end, end - tag->end))});
            cursor = closed ? end + 2 : end;
            tag = next;
        }
        menu.prompt += trim_end(text.substr(cursor));
        return menu;
    }
}
