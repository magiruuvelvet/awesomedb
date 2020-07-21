#include "any_comparator.hpp"

#include <string>
#include <cstdint>
#include <optional>

#include <QDateTime>

namespace utils {

#define COMPARATOR(type) \
    to_any_comparator<type>([](const type &l, const type &r){ return l == r; })

std::unordered_map<
    std::type_index, std::function<bool(const std::any&, const std::any&)>>
    any_comparator {

        // default data types
        to_any_comparator<void>([]{ return false; }), // void is never equal
        COMPARATOR(bool),
        COMPARATOR(float),
        COMPARATOR(double),
        COMPARATOR(std::uint8_t),
        COMPARATOR(std::uint16_t),
        COMPARATOR(std::uint32_t),
        COMPARATOR(std::uint64_t),
        COMPARATOR(std::int8_t),
        COMPARATOR(std::int16_t),
        COMPARATOR(std::int32_t),
        COMPARATOR(std::int64_t),
        COMPARATOR(std::string),

        // optional default data types
        COMPARATOR(std::optional<bool>),
        COMPARATOR(std::optional<float>),
        COMPARATOR(std::optional<double>),
        COMPARATOR(std::optional<std::uint8_t>),
        COMPARATOR(std::optional<std::uint16_t>),
        COMPARATOR(std::optional<std::uint32_t>),
        COMPARATOR(std::optional<std::uint64_t>),
        COMPARATOR(std::optional<std::int8_t>),
        COMPARATOR(std::optional<std::int16_t>),
        COMPARATOR(std::optional<std::int32_t>),
        COMPARATOR(std::optional<std::int64_t>),
        COMPARATOR(std::optional<std::string>),

        // Qt specific types
        COMPARATOR(QDateTime),
        COMPARATOR(QDate),
        COMPARATOR(QTime),
    };

bool compare_any(const std::any &l, const std::any &r, bool *success)
{
    if (const auto it = any_comparator.find(std::type_index(l.type()));
        it != any_comparator.cend())
    {
        if (success) (*success) = true;
        return it->second(l, r);
    }

    if (success) (*success) = false;
    return false;
}

}
