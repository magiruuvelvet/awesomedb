#pragma once

#include <string>
#include <cstdint>

struct DatabaseConfig final
{
    std::string host      {"127.0.0.1"};
    std::uint16_t port    {3306};
    std::string username;
    std::string password;
    std::string database;
};
