#pragma once

#include <any>
#include <typeindex>
#include <unordered_map>

namespace utils {

template<class T, class F>
static inline std::pair<const std::type_index, std::function<bool(const std::any&, const std::any&)>>
    to_any_comparator(F const &f)
{
    return {
        std::type_index(typeid(T)),
        [g = f](const std::any &l, const std::any &r)
        {
            if constexpr (std::is_void_v<T>)
            {
                return g();
            }
            else
            {
                return g(std::any_cast<T const&>(l), std::any_cast<T const&>(r));
            }
        }
    };
}

extern std::unordered_map<
    std::type_index, std::function<bool(const std::any&, const std::any&)>>
    any_comparator;

template<class T, class F>
inline void register_any_comparator(F const& f)
{
    any_comparator.insert(to_any_comparator<T>(f));
}

// example: register_any_comparator<std::string>([](const std::string &l, const std::string &r){ return l == r; });

// macro for easy data type registration when operator== is supported on the type
#define REGISTER_BASIC_ANY_COMPARATOR(type) \
    utils::register_any_comparator<type>([](const type &l, const type &r){ return l == r; })

// compares two std::any objects of the same type for equality
// throws an exception due to a failed std::any_cast<> if different
// data types where given as input
// the returned value is always the comparison result
// for explicit error handling to see if a data type wasn't registered
// use the success bool parameter
extern bool compare_any(const std::any &l, const std::any &r, bool *success = nullptr);

}
