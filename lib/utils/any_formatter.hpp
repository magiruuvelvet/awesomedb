#pragma once

#include <string>
#include <any>
#include <typeindex>
#include <unordered_map>

namespace utils {

template<class T, class F>
static inline std::pair<const std::type_index, std::function<std::string(const std::any&)>>
    to_any_formatter(F const &f)
{
    return {
        std::type_index(typeid(T)),
        [g = f](const std::any &any)
        {
            if constexpr (std::is_void_v<T>)
            {
                return std::string{};
            }
            else
            {
                return g(std::any_cast<T const&>(any));
            }
        }
    };
}

extern std::unordered_map<
    std::type_index, std::function<std::string(const std::any&)>>
    any_formatter;

template<class T, class F>
inline void register_any_formatter(F const& f)
{
    any_formatter.insert(to_any_formatter<T>(f));
}

// formats the given std::any object into a std::string using {fmtlib}
// for explicit error handling use the optimal success boolean parameter
extern const std::string format_any(const std::any &any, bool *success = nullptr);

}
