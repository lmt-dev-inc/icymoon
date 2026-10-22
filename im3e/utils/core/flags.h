#pragma once

#include <type_traits>

namespace im3e {

template <typename T>
inline constexpr bool IsFlagEnum = false;

template <typename T>
concept FlagEnum = std::is_enum_v<T> && IsFlagEnum<T>;

/// @brief Set of flags from a single-bit enum.
template <FlagEnum FlagT>
class Flags
{
public:
    using Bits = std::underlying_type_t<FlagT>;

    constexpr Flags() = default;
    constexpr Flags(FlagT flag)
      : m_bits(static_cast<Bits>(flag))
    {
    }

    constexpr static auto fromBits(Bits bits) -> Flags
    {
        Flags flags;
        flags.m_bits = bits;
        return flags;
    }

    constexpr auto has(FlagT flag) const -> bool { return (m_bits & static_cast<Bits>(flag)) != 0; }
    constexpr auto hasAll(Flags flags) const -> bool { return (m_bits & flags.m_bits) == flags.m_bits; }
    constexpr auto hasAny(Flags flags) const -> bool { return (m_bits & flags.m_bits) != 0; }
    constexpr auto isEmpty() const -> bool { return m_bits == 0; }

    constexpr auto operator|(Flags other) const -> Flags { return Flags::fromBits(m_bits | other.m_bits); }
    constexpr auto operator&(Flags other) const -> Flags { return Flags::fromBits(m_bits & other.m_bits); }
    constexpr auto operator|=(Flags other) -> Flags&
    {
        m_bits |= other.m_bits;
        return *this;
    }
    constexpr auto operator&=(Flags other) -> Flags&
    {
        m_bits &= other.m_bits;
        return *this;
    }

private:
    Bits m_bits{};
};

/// @brief Allows `E::A | E::B` to produce a Flags<E>.
template <FlagEnum FlagT>
constexpr auto operator|(FlagT lhs, FlagT rhs) -> Flags<FlagT>
{
    return Flags<FlagT>(lhs) | rhs;
}

}  // namespace im3e