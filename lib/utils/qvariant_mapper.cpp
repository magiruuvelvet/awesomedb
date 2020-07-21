#include "qvariant_mapper.hpp"

#include <string>
#include <cstdint>

#include <QDateTime>

namespace utils {

#define MAPPER(type, impl) \
    to_qvariant_mapper<type>([](type &target, const QVariant &var){ target = var.impl(); }), \
    to_qvariant_mapper<std::optional<type>>([](std::optional<type> &target, const QVariant &var){ target = var.impl(); })

std::unordered_map<
    std::type_index, std::function<void(std::any&, const QVariant&)>>
    qvariant_mapper {

        // default and optional data types
        MAPPER(bool, toBool),
        MAPPER(float, toFloat),
        MAPPER(double, toDouble),
        MAPPER(std::uint8_t, value<std::uint8_t>),
        MAPPER(std::uint16_t, value<std::uint16_t>),
        MAPPER(std::uint32_t, value<std::uint32_t>),
        MAPPER(std::uint64_t, value<std::uint64_t>),
        MAPPER(std::int8_t, value<std::int8_t>),
        MAPPER(std::int16_t, value<std::int16_t>),
        MAPPER(std::int32_t, value<std::int32_t>),
        MAPPER(std::int64_t, value<std::int16_t>),
        MAPPER(std::string, toString().toStdString),

        // Qt specific types
        MAPPER(QDateTime, toDateTime),
        MAPPER(QDate, toDate),
        MAPPER(QTime, toTime),
    };

bool any_from_qvariant(std::any &any, const QVariant &var)
{
    if (const auto it = qvariant_mapper.find(std::type_index(any.type()));
        it != qvariant_mapper.cend())
    {
        it->second(any, var);
        return true;
    }

    return false;
}

}
