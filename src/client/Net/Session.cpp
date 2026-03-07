//////////////////////////////////////////////////////////////////////////////
// This file is part of the Journey MMORPG client                           //
// Copyright © 2015-2016 Daniel Allendorf                                   //
//                                                                          //
// This program is free software: you can redistribute it and/or modify     //
// it under the terms of the GNU Affero General Public License as           //
// published by the Free Software Foundation, either version 3 of the       //
// License, or (at your option) any later version.                          //
//                                                                          //
// This program is distributed in the hope that it will be useful,          //
// but WITHOUT ANY WARRANTY; without even the implied warranty of           //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            //
// GNU Affero General Public License for more details.                      //
//                                                                          //
// You should have received a copy of the GNU Affero General Public License //
// along with this program.  If not, see <http://www.gnu.org/licenses/>.    //
//////////////////////////////////////////////////////////////////////////////
#include "Session.h"
#include "OutPacket.h"

#include "../Configuration.h"
#include "../Console.h"
#ifdef MS_PLATFORM_WASM
#include "../Util/Misc.h"
#endif
#include <iomanip>
#include <sstream>

namespace jrc
{
    namespace
    {
        int32_t next_net_debug_sequence()
        {
            static int32_t sequence = 0;
            return ++sequence;
        }

        uint16_t read_uint16_le(const int8_t* bytes, size_t offset)
        {
            return static_cast<uint16_t>(
                static_cast<uint16_t>(static_cast<uint8_t>(bytes[offset])) |
                (static_cast<uint16_t>(static_cast<uint8_t>(bytes[offset + 1])) << 8)
            );
        }

        int32_t read_int32_le(const int8_t* bytes, size_t offset)
        {
            return static_cast<int32_t>(
                static_cast<uint32_t>(static_cast<uint8_t>(bytes[offset])) |
                (static_cast<uint32_t>(static_cast<uint8_t>(bytes[offset + 1])) << 8) |
                (static_cast<uint32_t>(static_cast<uint8_t>(bytes[offset + 2])) << 16) |
                (static_cast<uint32_t>(static_cast<uint8_t>(bytes[offset + 3])) << 24)
            );
        }

        bool is_local_channel_address(const std::string& address)
        {
            return address == "127.0.0.1" || address == "0.0.0.0" || address == "localhost";
        }

        std::string resolve_channel_address(const char* address)
        {
            std::string target_address = (address != nullptr) ? address : "";
            if (!is_local_channel_address(target_address))
            {
                return target_address;
            }

            std::string configured_server_ip = Setting<MapleStoryServerIp>::get().load();
            if (configured_server_ip.empty())
            {
                return target_address;
            }

            Console::get().print(
                "Intercepted local channel IP " + target_address +
                ", substituting with " + configured_server_ip
            );
            return configured_server_ip;
        }
    }

    Session::Session()
    {
        connected = false;
        length    = 0;
        pos       = 0;
    }

    Session::~Session()
    {
        if (connected)
        {
            socket.close();
        }
    }

    bool Session::init(const char* host, const char* port)
    {
#ifdef MS_PLATFORM_WASM
        std::string final_host = host;
        if (final_host == "0.0.0.0")
        {
            final_host = getBrowserHostname();
            Console::get().print("Host is 0.0.0.0, resolving to: " + final_host);
        }
        Console::get().print("Connecting to " + final_host + ":" + port);
        connected = socket.open(final_host.c_str(), port);
#else
        // Connect to the server.
        connected = socket.open(host, port);
#endif

        if (connected)
        {
            // Read keys neccessary for communicating with the server.
            cryptography = { socket.get_buffer() };
        }
#ifdef MS_PLATFORM_WASM
        else
        {
            Console::get().print("Failed to connect");
        }
#endif

        return connected;
    }

    Error Session::init()
    {
        std::string HOST = Setting<MapleStoryServerIp>::get().load();
        std::string PORT = Setting<MapleStoryServerPort>::get().load();

        if (!init(HOST.c_str(), PORT.c_str()))
        {
            return Error::CONNECTION;
        }

        return Error::NONE;
    }

    void Session::reconnect(const char* address, const char* port)
    {
        // Close the current connection and open a new one.
        bool success = socket.close();

        if (success)
        {
            std::string target_address = resolve_channel_address(address);
            const char* target_port = (port != nullptr) ? port : "";
            init(target_address.c_str(), target_port);
        }
        else
        {
            connected = false;
        }
    }

    void Session::process(const int8_t* bytes, size_t available)
    {
        if (pos == 0)
        {
            // Pos is 0, meaning this is the start of a new packet.
            // Start by determining length.
            length = cryptography.check_length(bytes);
            // Reading the length means we processed the header.
            // Move forward by the header length.
            bytes = bytes + HEADER_LENGTH;
            available -= HEADER_LENGTH;
        }

        // Determine how much we can write. Write data into the buffer.
        size_t towrite = length - pos;

        if (towrite > available)
        {
            towrite = available;
        }

        memcpy(buffer + pos, bytes, towrite);
        pos += towrite;

        // Check if the current packet has been fully processed.
        if (pos >= length)
        {
            cryptography.decrypt(buffer, length);

            try
            {
                packetswitch.forward(buffer, length);
            }
            catch (const PacketError& err)
            {
                Console::get().print(err.what());
            }

            pos    = 0;
            length = 0;

            // Check if there is more available.
            size_t remaining = available - towrite;

            if (remaining >= MIN_PACKET_LENGTH)
            {
                // More packets are available, so we start over.
                process(bytes + towrite, remaining);
            }
        }
    }

    void Session::write(int8_t* packet_bytes, size_t packet_length)
    {
        if (!connected)
        {
            return;
        }

        if (packet_length >= 2)
        {
            uint16_t opcode =
                static_cast<uint16_t>(static_cast<uint8_t>(packet_bytes[0])) |
                (static_cast<uint16_t>(static_cast<uint8_t>(packet_bytes[1])) << 8);

            if (opcode == OutPacket::TALK_TO_NPC && packet_length >= 6)
            {
                int32_t oid = read_int32_le(packet_bytes, 2);
                Console::get().print(
                    "[npc-debug] C->S TALK_TO_NPC seq=" + std::to_string(next_net_debug_sequence()) +
                    " oid=" + std::to_string(oid) +
                    " len=" + std::to_string(packet_length)
                );
            }
            else if (opcode == OutPacket::NPC_TALK_MORE && packet_length >= 4)
            {
                int8_t lastmsg = packet_bytes[2];
                int8_t response = packet_bytes[3];
                std::string details;

                if (lastmsg == 4 && packet_length >= 8)
                {
                    int32_t selection = read_int32_le(packet_bytes, 4);
                    details = " selection=" + std::to_string(selection);
                }
                else if (lastmsg == 2 && packet_length >= 6)
                {
                    uint16_t text_length = read_uint16_le(packet_bytes, 4);
                    details = " text_len=" + std::to_string(text_length);
                }

                Console::get().print(
                    "[npc-debug] C->S NPC_TALK_MORE(seq=" + std::to_string(next_net_debug_sequence()) +
                    ") lastmsg=" + std::to_string(lastmsg) +
                    " response=" + std::to_string(response) +
                    " len=" + std::to_string(packet_length) +
                    details
                );
            }
            else if (opcode == OutPacket::NPC_SHOP_ACTION && packet_length >= 3)
            {
                int8_t mode = packet_bytes[2];
                Console::get().print(
                    "[npc-debug] C->S NPC_SHOP_ACTION seq=" + std::to_string(next_net_debug_sequence()) +
                    " mode=" + std::to_string(mode) +
                    " len=" + std::to_string(packet_length)
                );
            }

            if (opcode == OutPacket::CREATE_CHAR)
            {
                std::ostringstream hex;
                hex << std::uppercase << std::hex << std::setfill('0');
                for (size_t i = 0; i < packet_length; ++i)
                {
                    hex << std::setw(2)
                        << static_cast<unsigned int>(static_cast<uint8_t>(packet_bytes[i]));
                }

                Console::get().print(
                    "[NET] CREATE_CHAR len=" + std::to_string(packet_length) +
                    " payload=" + hex.str()
                );
            }
        }

        int8_t header[HEADER_LENGTH];
        cryptography.create_header(header, packet_length);
        cryptography.encrypt(packet_bytes, packet_length);

        socket.dispatch(header, HEADER_LENGTH);
        socket.dispatch(packet_bytes, packet_length);
    }

    void Session::read()
    {
        // Check if a packet has arrived. Handle if data is sufficient:
        //     4 bytes(header) + 2 bytes(opcode) = 6.
        size_t result = socket.receive(&connected);

        if (result >= MIN_PACKET_LENGTH || length > 0)
        {
            // Retrieve buffer from the socket and process it.
            const int8_t* bytes = socket.get_buffer();
            process(bytes, result);
        }
    }

    bool Session::is_connected() const
    {
        return connected;
    }
}
