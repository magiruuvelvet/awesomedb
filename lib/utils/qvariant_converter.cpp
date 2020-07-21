#include "qvariant_converter.hpp"

#include <string>
#include <cstdint>
#include <optional>

#include <QDateTime>

namespace utils {

#define MAPPER_BASIC(type) \
    to_qvariant_converter<type>([](const type &source){ return QVariant::fromValue(source); })

#define MAPPER_CUSTOM(type, impl) \
    to_qvariant_converter<type>([](const type &source){ return impl; })

#define MAPPER_BASIC_OPTIONAL(type)                     \
    to_qvariant_converter<type>([](const type &source){ \
        if (!source.has_value())                        \
            return QVariant();                          \
        return QVariant::fromValue(source.value()); })

std::unordered_map<
    std::type_index, std::function<QVariant(const std::any&)>>
    qvariant_converter {

        // default data types
        MAPPER_BASIC(bool),
        MAPPER_BASIC(float),
        MAPPER_BASIC(double),
        MAPPER_BASIC(std::uint8_t),
        MAPPER_BASIC(std::uint16_t),
        MAPPER_BASIC(std::uint32_t),
        MAPPER_BASIC(std::uint64_t),
        MAPPER_BASIC(std::int8_t),
        MAPPER_BASIC(std::int16_t),
        MAPPER_BASIC(std::int32_t),
        MAPPER_BASIC(std::int64_t),
        MAPPER_CUSTOM(std::string, QVariant::fromValue(QString::fromStdString(source))),

        // optional default data types
        MAPPER_BASIC_OPTIONAL(std::optional<bool>),
        MAPPER_BASIC_OPTIONAL(std::optional<float>),
        MAPPER_BASIC_OPTIONAL(std::optional<double>),
        MAPPER_BASIC_OPTIONAL(std::optional<std::uint8_t>),
        MAPPER_BASIC_OPTIONAL(std::optional<std::uint16_t>),
        MAPPER_BASIC_OPTIONAL(std::optional<std::uint32_t>),
        MAPPER_BASIC_OPTIONAL(std::optional<std::uint64_t>),
        MAPPER_BASIC_OPTIONAL(std::optional<std::int8_t>),
        MAPPER_BASIC_OPTIONAL(std::optional<std::int16_t>),
        MAPPER_BASIC_OPTIONAL(std::optional<std::int32_t>),
        MAPPER_BASIC_OPTIONAL(std::optional<std::int64_t>),
        to_qvariant_converter<std::optional<std::string>>([](const std::optional<std::string> &source){
            if (!source.has_value())
                return QVariant();
            return QVariant::fromValue(QString::fromStdString(source.value()));
        }),

        // Qt specific types
        MAPPER_BASIC(QDateTime),
        MAPPER_BASIC(QDate),
        MAPPER_BASIC(QTime),
    };

QVariant qvariant_from_any(const std::any &any, bool *success)
{
    if (const auto it = qvariant_converter.find(std::type_index(any.type()));
        it != qvariant_converter.cend())
    {
        if (success) (*success) = true;
        return it->second(any);
    }

    if (success) (*success) = false;
    return QVariant();
}

}
