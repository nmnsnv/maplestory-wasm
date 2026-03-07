#include "MapEffect.h"

#include "../../Audio/Audio.h"
#include "../../Console.h"
#include "../../Constants.h"

#include "nlnx/nx.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <climits>
#include <vector>

namespace jrc
{
    namespace
    {
        std::vector<std::string> split_path(const std::string& path)
        {
            std::vector<std::string> parts;
            std::string current;
            for (char ch : path)
            {
                if (ch == '/')
                {
                    if (!current.empty())
                    {
                        parts.push_back(current);
                        current.clear();
                    }
                    continue;
                }

                current.push_back(ch);
            }

            if (!current.empty())
            {
                parts.push_back(current);
            }

            return parts;
        }

        std::vector<std::string> token_variants(const std::string& token)
        {
            std::vector<std::string> variants;
            variants.reserve(4);
            variants.push_back(token);

            constexpr size_t IMG_SUFFIX_LENGTH = 4;
            if (token.size() > IMG_SUFFIX_LENGTH &&
                token.compare(token.size() - IMG_SUFFIX_LENGTH, IMG_SUFFIX_LENGTH, ".img") == 0)
            {
                variants.push_back(token.substr(0, token.size() - IMG_SUFFIX_LENGTH));
            }
            else
            {
                variants.push_back(token + ".img");
            }

            if (token == "Effect")
            {
                variants.push_back("effect");
            }
            else if (token == "effect")
            {
                variants.push_back("Effect");
            }

            return variants;
        }

        nl::node resolve_tokens(nl::node root, const std::vector<std::string>& tokens)
        {
            std::vector<nl::node> candidates;
            candidates.push_back(root);

            for (const std::string& token : tokens)
            {
                std::vector<nl::node> next_candidates;
                std::vector<std::string> options = token_variants(token);

                for (const nl::node& candidate : candidates)
                {
                    for (const std::string& option : options)
                    {
                        nl::node child = candidate[option];
                        if (child)
                        {
                            next_candidates.push_back(child);
                        }
                    }
                }

                if (next_candidates.empty())
                {
                    return {};
                }

                candidates.swap(next_candidates);
            }

            for (const nl::node& candidate : candidates)
            {
                if (candidate)
                {
                    return candidate;
                }
            }

            return {};
        }

        nl::node resolve_effect_node(const std::string& path)
        {
            std::vector<std::string> tokens = split_path(path);
            if (tokens.empty())
            {
                return {};
            }

            std::vector<std::vector<std::string>> token_sets;
            token_sets.push_back(tokens);

            if (tokens.front() == "Effect")
            {
                std::vector<std::string> without_prefix(tokens.begin() + 1, tokens.end());
                if (!without_prefix.empty())
                {
                    token_sets.push_back(without_prefix);
                }
            }

            for (const std::vector<std::string>& token_set : token_sets)
            {
                nl::node node = resolve_tokens(nl::nx::effect, token_set);
                if (node)
                {
                    return node;
                }

                node = resolve_tokens(nl::nx::map["Effect.img"], token_set);
                if (node)
                {
                    return node;
                }
            }

            return {};
        }

        bool has_animation_frames(nl::node node)
        {
            if (!node)
            {
                return false;
            }

            if (node.data_type() == nl::node::type::bitmap)
            {
                return true;
            }

            for (auto child : node)
            {
                if (child.data_type() == nl::node::type::bitmap)
                {
                    return true;
                }
            }

            return false;
        }

        nl::node find_animation_descendant(nl::node node, int depth)
        {
            if (!node || depth < 0)
            {
                return {};
            }

            if (has_animation_frames(node))
            {
                return node;
            }

            for (auto child : node)
            {
                nl::node found = find_animation_descendant(child, depth - 1);
                if (found)
                {
                    return found;
                }
            }

            return {};
        }

        nl::node resolve_animation_node(const std::string& path)
        {
            nl::node node = resolve_effect_node(path);
            if (!has_animation_frames(node))
            {
                node = find_animation_descendant(node, 6);
            }

            return has_animation_frames(node) ? node : nl::node{};
        }

        nl::node resolve_sound_node(const std::string& path)
        {
            if (path.empty())
            {
                return {};
            }

            static constexpr const char* SOUND_PREFIX = "Sound/";
            static constexpr size_t SOUND_PREFIX_LENGTH = 6;
            if (path.compare(0, SOUND_PREFIX_LENGTH, SOUND_PREFIX) == 0)
            {
                nl::node node = nl::nx::sound.resolve(path.substr(SOUND_PREFIX_LENGTH));
                if (node.data_type() == nl::node::type::audio)
                {
                    return node;
                }
            }

            static constexpr const char* EFFECT_PREFIX = "Effect/";
            static constexpr size_t EFFECT_PREFIX_LENGTH = 7;
            if (path.compare(0, EFFECT_PREFIX_LENGTH, EFFECT_PREFIX) == 0)
            {
                nl::node node = nl::nx::effect.resolve(path.substr(EFFECT_PREFIX_LENGTH));
                if (node.data_type() == nl::node::type::audio)
                {
                    return node;
                }
            }

            nl::node effect_node = resolve_effect_node(path);
            if (effect_node.data_type() == nl::node::type::audio)
            {
                return effect_node;
            }

            nl::node sound_node = nl::nx::sound.resolve(path);
            if (sound_node.data_type() == nl::node::type::audio)
            {
                return sound_node;
            }

            return {};
        }

        bool has_integer_field(nl::node node, const char* field_name)
        {
            return node[field_name].data_type() == nl::node::type::integer;
        }

        bool try_parse_int(const std::string& token, int32_t& value)
        {
            if (token.empty())
            {
                return false;
            }

            size_t index = 0;
            if (token[0] == '+' || token[0] == '-')
            {
                index = 1;
            }

            if (index >= token.size())
            {
                return false;
            }

            for (; index < token.size(); ++index)
            {
                if (!std::isdigit(static_cast<unsigned char>(token[index])))
                {
                    return false;
                }
            }

            try
            {
                value = std::stoi(token);
            }
            catch (...)
            {
                return false;
            }

            return true;
        }

        struct SceneVisualSpec
        {
            int32_t start_ms = 0;
            int32_t duration_ms = -1;
            int32_t z = 0;
            int32_t index = INT32_MAX;
            Point<int16_t> start_offset = {};
            Point<int16_t> end_offset = {};
            nl::node animation_node = {};
        };

        struct SceneSoundSpec
        {
            std::string path;
            int32_t start_ms = 0;
            int32_t index = 0;
        };

        bool parse_scene_visuals(nl::node scene_node, std::vector<SceneVisualSpec>& output)
        {
            output.clear();

            for (auto action : scene_node)
            {
                int32_t action_index = INT32_MAX;
                if (!try_parse_int(action.name(), action_index))
                {
                    continue;
                }

                int32_t type = action["type"];
                if (type != 0)
                {
                    continue;
                }

                std::string visual_path = action["visual"];
                if (visual_path.empty())
                {
                    continue;
                }

                nl::node visual_node = resolve_animation_node(visual_path);
                if (!visual_node)
                {
                    continue;
                }

                int32_t start_ms = std::max<int32_t>(0, action["start"]);
                int32_t duration_ms = -1;
                if (has_integer_field(action, "duration"))
                {
                    int32_t parsed_duration = action["duration"];
                    if (parsed_duration > 0)
                    {
                        duration_ms = parsed_duration;
                    }
                }

                int32_t x = action["x"];
                int32_t y = action["y"];
                int32_t x1 = has_integer_field(action, "x1") ? static_cast<int32_t>(action["x1"]) : x;
                int32_t y1 = has_integer_field(action, "y1") ? static_cast<int32_t>(action["y1"]) : y;
                int32_t z = has_integer_field(action, "z") ? static_cast<int32_t>(action["z"]) : 0;

                SceneVisualSpec spec;
                spec.start_ms = start_ms;
                spec.duration_ms = duration_ms;
                spec.z = z;
                spec.index = action_index;
                spec.start_offset = Point<int16_t>(static_cast<int16_t>(x), static_cast<int16_t>(y));
                spec.end_offset = Point<int16_t>(static_cast<int16_t>(x1), static_cast<int16_t>(y1));
                spec.animation_node = visual_node;
                output.push_back(spec);
            }

            std::sort(
                output.begin(),
                output.end(),
                [](const SceneVisualSpec& left, const SceneVisualSpec& right) {
                    if (left.z != right.z)
                    {
                        return left.z < right.z;
                    }
                    if (left.start_ms != right.start_ms)
                    {
                        return left.start_ms < right.start_ms;
                    }
                    return left.index < right.index;
                }
            );

            return !output.empty();
        }

        void parse_scene_sounds(nl::node scene_node, std::vector<SceneSoundSpec>& output)
        {
            output.clear();

            for (auto action : scene_node)
            {
                int32_t action_index = INT32_MAX;
                if (!try_parse_int(action.name(), action_index))
                {
                    continue;
                }

                std::string sound_path = action["sound"];
                if (sound_path.empty())
                {
                    continue;
                }

                output.push_back(
                    {
                        sound_path,
                        std::max<int32_t>(0, action["start"]),
                        action_index
                    }
                );
            }

            std::sort(
                output.begin(),
                output.end(),
                [](const SceneSoundSpec& left, const SceneSoundSpec& right) {
                    if (left.start_ms != right.start_ms)
                    {
                        return left.start_ms < right.start_ms;
                    }
                    return left.index < right.index;
                }
            );
        }
    }

    MapEffect::MapEffect(const std::string& path)
        : finished(false),
          scene_mode(false),
          elapsed_ms(0),
          position(Constants::viewwidth() / 2, 250),
          scene_origin(Constants::viewwidth() / 2, Constants::viewheight() / 2)
    {
        nl::node effect_node = resolve_effect_node(path);
        if (!effect_node)
        {
            Console::get().print("[MapEffect] Missing NX effect path: " + path);
            finished = true;
            return;
        }

        if (!has_animation_frames(effect_node))
        {
            std::vector<SceneVisualSpec> scene_specs;
            if (parse_scene_visuals(effect_node, scene_specs))
            {
                scene_visuals.clear();
                scene_visuals.reserve(scene_specs.size());
                for (const SceneVisualSpec& spec : scene_specs)
                {
                    SceneVisual visual;
                    visual.animation = Animation(spec.animation_node);
                    visual.start_ms = spec.start_ms;
                    visual.duration_ms = spec.duration_ms;
                    visual.z = spec.z;
                    visual.start_offset = spec.start_offset;
                    visual.end_offset = spec.end_offset;
                    scene_visuals.push_back(std::move(visual));
                }

                std::vector<SceneSoundSpec> scene_sound_specs;
                parse_scene_sounds(effect_node, scene_sound_specs);
                scene_sounds.clear();
                scene_sounds.reserve(scene_sound_specs.size());
                for (const SceneSoundSpec& sound_spec : scene_sound_specs)
                {
                    scene_sounds.push_back(
                        {
                            sound_spec.path,
                            sound_spec.start_ms,
                            sound_spec.index,
                            false
                        }
                    );
                }
                scene_mode = true;
                return;
            }
        }

        if (!has_animation_frames(effect_node))
        {
            nl::node fallback_animation = find_animation_descendant(effect_node, 6);
            if (fallback_animation)
            {
                effect_node = fallback_animation;
            }
        }

        if (!has_animation_frames(effect_node))
        {
            Console::get().print(
                "[MapEffect] Resolved node has no drawable frames for path: " + path
            );
            finished = true;
            return;
        }

        effect = Animation(effect_node);
    }

    MapEffect::MapEffect()
        : finished(true),
          scene_mode(false),
          elapsed_ms(0),
          scene_origin(Constants::viewwidth() / 2, Constants::viewheight() / 2)
    {}

    void MapEffect::draw_scene() const
    {
        for (const SceneVisual& visual : scene_visuals)
        {
            if (elapsed_ms < visual.start_ms)
            {
                continue;
            }

            int32_t local_elapsed = elapsed_ms - visual.start_ms;
            if (visual.duration_ms > 0 && local_elapsed > visual.duration_ms)
            {
                continue;
            }

            Point<int16_t> offset = visual.start_offset;
            if (visual.duration_ms > 0)
            {
                int32_t clamped = std::min(local_elapsed, visual.duration_ms);
                int32_t dx = static_cast<int32_t>(visual.end_offset.x()) - static_cast<int32_t>(visual.start_offset.x());
                int32_t dy = static_cast<int32_t>(visual.end_offset.y()) - static_cast<int32_t>(visual.start_offset.y());

                int16_t x = static_cast<int16_t>(
                    static_cast<int32_t>(visual.start_offset.x()) + (dx * clamped) / visual.duration_ms
                );
                int16_t y = static_cast<int16_t>(
                    static_cast<int32_t>(visual.start_offset.y()) + (dy * clamped) / visual.duration_ms
                );
                offset = Point<int16_t>(x, y);
            }

            visual.animation.draw(DrawArgument(scene_origin + offset), 1.0f);
        }
    }

    void MapEffect::draw() const
    {
        if (finished)
        {
            return;
        }

        if (scene_mode)
        {
            draw_scene();
            return;
        }

        effect.draw(DrawArgument(position), 1.0f);
    }

    void MapEffect::update_scene()
    {
        bool has_active_or_future_activity = false;
        for (SceneVisual& visual : scene_visuals)
        {
            if (elapsed_ms < visual.start_ms)
            {
                has_active_or_future_activity = true;
                continue;
            }

            int32_t local_elapsed = elapsed_ms - visual.start_ms;
            if (visual.duration_ms > 0 && local_elapsed > visual.duration_ms)
            {
                continue;
            }

            has_active_or_future_activity = true;
            visual.animation.update(Constants::TIMESTEP);
        }

        for (SceneSound& sound_event : scene_sounds)
        {
            if (sound_event.played)
            {
                continue;
            }

            if (elapsed_ms >= sound_event.start_ms)
            {
                nl::node sound_node = resolve_sound_node(sound_event.path);
                if (sound_node.data_type() == nl::node::type::audio)
                {
                    Sound(sound_node).play();
                }
                else
                {
                    Console::get().print(
                        "[MapEffect] Intro scene sound could not be resolved: " + sound_event.path
                    );
                }
                sound_event.played = true;
                continue;
            }

            has_active_or_future_activity = true;
        }

        elapsed_ms += Constants::TIMESTEP;
        finished = !has_active_or_future_activity;
    }

    void MapEffect::update()
    {
        if (finished)
        {
            return;
        }

        if (scene_mode)
        {
            update_scene();
            return;
        }

        finished = effect.update(6);
    }

    bool MapEffect::blocks_player_input() const
    {
        return scene_mode && !finished;
    }
}
