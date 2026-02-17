#pragma once

#include <string>

#include <data.hpp>

class Serializer
{
public:
    static bool toJson(const PacketData& packet, std::string& outJson);
};
