#pragma once

#include <algorithm>
#include <string_view>
#include <sstream>

namespace utils {

template<typename ListType, typename Value = typename ListType::value_type>
const std::string list_join(const ListType &list, const std::string_view &delimiter)
{
    std::ostringstream os;
    auto b = std::begin(list);
    auto e = std::end(list);

    if (b != e)
    {
        std::copy(b, std::prev(e), std::ostream_iterator<Value>(os, delimiter.data()));
        b = std::prev(e);
    }
    if (b != e)
    {
        os << *b;
    }

    return os.str();
}

}
