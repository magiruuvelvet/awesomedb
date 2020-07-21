#pragma once

#include <any>
#include <typeindex>
#include <unordered_map>

#include <QVariant>

namespace utils {

template<class T, class F>
static inline std::pair<const std::type_index, std::function<void(std::any&, const QVariant&)>>
    to_qvariant_mapper(F const &f)
{
    return {
        std::type_index(typeid(T)),
        [g = f](std::any &target, const QVariant &variant)
        {
            if constexpr (std::is_void_v<T>)
            {
                target.reset();
            }
            else
            {
                g(std::any_cast<T&>(target), variant);
            }
        }
    };
}

extern std::unordered_map<
    std::type_index, std::function<void(std::any&, const QVariant&)>>
    qvariant_mapper;

template<class T, class F>
inline void register_qvariant_mapper(F const& f)
{
    qvariant_mapper.insert(to_qvariant_mapper<T>(f));
}

// assigns the QVariant value into a std::any object using a map
// new types can be registered with register_qvariant_mapper()
// the std::any object must already have a data type assigned
// returns true if the data type was registered and converted
// returns false when the data type is unknown and no conversion happened
extern bool any_from_qvariant(std::any &any, const QVariant &var);

}
