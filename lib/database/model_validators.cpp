#include "model.hpp"

bool Model::validate_varchar_length(std::uint64_t length, const std::string &text)
{
    return text.size() <= length;
}

bool Model::validate_mediumtext_length(const std::string &text)
{
    return text.size() <= 0xffffff; // 16,777,215 bytes
}
