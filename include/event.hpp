#pragma once
#include <cmath>
#include <array>
#include <concepts>
#include <optional>
#include <variant>

#include <data.hpp>

class Event
{
private:
    duration_t _elapsed {duration_t::zero()};

public:
    void tick(duration_t dt)
    {
        this->_elapsed += dt;
    }

    duration_t elapsed(void) const
    {
        return this->_elapsed;
    }
};

class FreeFall final : public Event
{
private:
    static constexpr std::chrono::microseconds TRIGGER_TIME {200};
    static constexpr float FREE_FALL_THRESHOLD {0.4f};

public:
    FreeFall(void) = default;

    bool is_free_fall(void) const
    {
        return this->elapsed() >= this->TRIGGER_TIME;
    }
    static bool is_free_fall(const std::optional<FreeFall>& free_fall)
    {
        return free_fall.has_value() && free_fall->is_free_fall();
    }

    static bool require(float mag_g)
    {
        return mag_g < FreeFall::FREE_FALL_THRESHOLD;
    }

    static void update(std::optional<FreeFall>& free_fall, const std::array<float, 3>& accel, duration_t dt)
    {
        const auto& [ax_g, ay_g, az_g] = accel;
        const float mag_g {std::sqrt(ax_g*ax_g + ay_g*ay_g + az_g*az_g)};
        if(!FreeFall::require(mag_g))
        {
            free_fall = std::nullopt;
        }
        else if(free_fall.has_value())
        {
            free_fall->tick(dt);
        }
        else
        {
            free_fall = FreeFall();
        }
    }
};

class TiltChange final : public Event
{
private:
    static constexpr std::chrono::microseconds TILT_WINDOW_TIME {500};

    bool _was_lying_at_impact;
    bool _is_tilt_change {false};

public:
    TiltChange(bool is_lying_now):
        _was_lying_at_impact(is_lying_now)
    {

    }

    bool is_tilt_change(void) const
    {
        return this->_is_tilt_change;
    }
    static bool is_tilt_change(const std::optional<TiltChange>& tilt_change)
    {
        return tilt_change.has_value() && tilt_change->is_tilt_change();
    }

    bool keep(void)
    {
        return this->elapsed() <= TILT_WINDOW_TIME;
    }

    void check(bool is_lying_now)
    {
        this->_is_tilt_change |= is_lying_now != this->_was_lying_at_impact;
    }

    static void update(std::optional<TiltChange>& tilt_change, bool is_lying_now, duration_t dt)
    {
        if(tilt_change.has_value())
        {
            tilt_change->tick(dt);
            if(tilt_change->keep())
            {
                tilt_change = std::nullopt;
            }
            else
            {
                tilt_change->check(is_lying_now);
            }
        }
    }
};

class Impact final : public Event
{
private:
    static constexpr float IMPACT_THRESHOLD {2.5f};
    static constexpr float IS_LYING_THRESHOLD {0.5f};

    // Included inside because impact event is always a precondition for tilt_change event
    std::optional<TiltChange> tilt_change;

public:
    Impact(bool is_lying_now):
        tilt_change(TiltChange(is_lying_now))
    {

    }

    bool is_impact(void) const
    {
        return true;
    }
    static bool is_impact(const std::optional<Impact>& impact)
    {
        return impact.has_value() && impact->is_impact();
    }

    bool is_tilt_change(void) const
    {
        return TiltChange::is_tilt_change(this->tilt_change);
    }
    static bool is_tilt_change(const std::optional<Impact>& impact)
    {
        return impact.has_value() && impact->is_tilt_change();
    }

    static bool trigger(float mag_g)
    {
        return mag_g > IMPACT_THRESHOLD;
    }

    static void update(std::optional<Impact>& impact, float mag_g, bool is_lying_now, duration_t dt)
    {
        if(impact.has_value())
        {
            impact->tick(dt);
            TiltChange::update(impact->tilt_change, is_lying_now, dt);
        }
        else if(Impact::trigger(mag_g))
        {
            impact = {Impact(is_lying_now)};
        }
    }
};

class PostFall final : public Event
{
public:
    PostFall(void) = default;

    static bool trigger(const std::optional<FreeFall>& free_fall, const std::optional<Impact>& impact, bool is_lying_now)
    {
        const int event_count = 
            static_cast<std::uint8_t>(FreeFall::is_free_fall(free_fall))
            + static_cast<std::uint8_t>(Impact::is_impact(impact))
            + static_cast<std::uint8_t>(Impact::is_tilt_change(impact));
        return event_count >= 2 && is_lying_now;
    }

    static void update(std::optional<PostFall>& post_fall, const std::optional<FreeFall>& free_fall, const std::optional<Impact>& impact, bool is_lying_now, duration_t dt)
    {
        if(post_fall.has_value())
        {
            post_fall->tick(dt);
        }
        else if(PostFall::trigger(free_fall, impact, is_lying_now))
        {
            post_fall = PostFall();
        }
    }
};