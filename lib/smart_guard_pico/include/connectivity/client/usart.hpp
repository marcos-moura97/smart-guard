#pragma once

#include "connectivity/send.hpp"
#include "data.hpp"

class Usart : public Send<PacketData>
{
public:
    virtual bool send(PacketData packet) override
    {
        std::string csv {packet.format_csv()};
        std::printf("%s", csv.c_str());
        return true;
    }
};