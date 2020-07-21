#pragma once

#include <any>
#include <typeindex>
#include <unordered_map>

#include <QVariant>

namespace utils {

template<class T, class F>
static inline std::pair<const std::type_index, std::function<QVariant(const std::any&)>>
    to_qvariant_converter(F const &f)
{
    return {
        std::type_index(typeid(T)),
        [g = f](const std::any &source)
        {
            if constexpr (std::is_void_v<T>)
            {
                return QVariant();
            }
            else
            {
                return g(std::any_cast<T const&>(source));
            }
        }
    };
}

extern std::unordered_map<
    std::type_index, std::function<QVariant(const std::any&)>>
    qvariant_converter;

template<class T, class F>
inline void register_qvariant_converter(F const& f)
{
    qvariant_converter.insert(to_qvariant_converter<T>(f));
}

// takes the value from a std::any object and assigns it into a QVariant
// using a map, new types can be registered with register_qvariant_converter()
// returns an invalid QVariant on void types or when the data type wasn't registered
// if you need explicit error handling use the optimal success bool parameter
extern QVariant qvariant_from_any(const std::any &any, bool *success = nullptr);

}
