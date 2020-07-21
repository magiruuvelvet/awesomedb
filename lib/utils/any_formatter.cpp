#include "any_formatter.hpp"

#include <fmt/format.h>

#include <cstdint>
#include <optional>

#include <QDateTime>

#define QT_FMT(type, func)                                  \
template<> struct fmt::formatter<type> {                    \
    constexpr auto parse(format_parse_context &ctx)         \
    { return ctx.begin(); }                                 \
    template<typename FormatContext>                        \
    auto format(const type &var, FormatContext &ctx) {      \
        return format_to(ctx.out(), "{}", var.func); } }    \

QT_FMT(QDateTime, toString(Qt::ISODate).toStdString());
QT_FMT(QDate, toString(Qt::ISODate).toStdString());
QT_FMT(QTime, toString(Qt::ISODate).toStdString());

namespace utils {

#define FORMATTER(type)                                                            \
    to_any_formatter<type>([](const type &any){ return fmt::format("{}", any); }), \
    to_any_formatter<std::optional<type>>([](const std::optional<type> &any){      \
        if (!any.has_value())                                                      \
            return std::string{"{NULL}"};                                          \
        return fmt::format("{}", any.value());                                     \
    })

std::unordered_map<
    std::type_index, std::function<std::string(const std::any&)>>
    any_formatter {

        // default data types
        FORMATTER(bool),
        FORMATTER(float),
        FORMATTER(double),
        FORMATTER(std::uint8_t),
        FORMATTER(std::uint16_t),
        FORMATTER(std::uint32_t),
        FORMATTER(std::uint64_t),
        FORMATTER(std::int8_t),
        FORMATTER(std::int16_t),
        FORMATTER(std::int32_t),
        FORMATTER(std::int64_t),
        FORMATTER(std::string),

        // Qt specific types
        FORMATTER(QDateTime),
        FORMATTER(QDate),
        FORMATTER(QTime),
    };

const std::string format_any(const std::any &any, bool *success)
{
    if (const auto it = any_formatter.find(std::type_index(any.type()));
        it != any_formatter.cend())
    {
        if (success) (*success) = true;
        return it->second(any);
    }

    if (success) (*success) = false;
    return {};
}

}
