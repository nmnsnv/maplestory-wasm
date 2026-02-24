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
        std::string HOST = Setting<ServerIP>::get().load();
        std::string PORT = "8484";

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
            init(address, port);
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
