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
#include "SocketWinsock.h"
#ifndef JOURNEY_USE_ASIO

#ifdef MS_PLATFORM_WASM
#include "SockInternal.h"
#include "../Console.h"
#else
#include <WinSock2.h>
#include <ws2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#endif

namespace jrc
{
    bool SocketWinsock::open(const char* iaddr, const char* port)
    {
#ifdef MS_PLATFORM_WASM
        Console::get().print("Opening connection: " + std::string(iaddr) + ":" + std::string(port));
        sock = ws_connect(std::string(iaddr), std::string(port));
        int result = ws_recv(sock, (char*)buffer, 32, (size_t)-1);
        return result == HANDSHAKE_LEN;
#else
        WSADATA wsa_info;
        sock = INVALID_SOCKET;

        struct addrinfo *addr_info = NULL;
        struct addrinfo *ptr = NULL;
        struct addrinfo hints;

        int result = WSAStartup(MAKEWORD(2, 2), &wsa_info);
        if (result != 0)
        {
            return false;
        }

        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        result = getaddrinfo(iaddr, port, &hints, &addr_info);
        if (result != 0)
        {
            WSACleanup();
            return false;
        }

        for (ptr = addr_info; ptr != NULL; ptr = ptr->ai_next)
        {
            sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
            if (sock == INVALID_SOCKET)
            {
                WSACleanup();
                return false;
            }
            result = connect(sock, ptr->ai_addr, (int)ptr->ai_addrlen);
            if (result == SOCKET_ERROR)
            {
                closesocket(sock);
                sock = INVALID_SOCKET;
                continue;
            }
            break;
        }

        freeaddrinfo(addr_info);

        if (sock == INVALID_SOCKET)
        {
            WSACleanup();
            return false;
        }

        result = recv(sock, (char*)buffer, 32, 0);
        if (result == HANDSHAKE_LEN)
        {
            return true;
        }
        else
        {
            WSACleanup();
            return false;
        }
#endif
    }

    bool SocketWinsock::close()
    {
#ifdef MS_PLATFORM_WASM
        Console::get().print("Closing connection");
        int error = ws_closesocket(sock);
        return error != WS_SOCK_ERROR;
#else
        int error = closesocket(sock);
        WSACleanup();
        return error != SOCKET_ERROR;
#endif
    }

    bool SocketWinsock::dispatch(const int8_t* bytes, size_t length) const
    {
#ifdef MS_PLATFORM_WASM
        return ws_send(sock, (char*)bytes, static_cast<int>(length), 0) != WS_SOCK_ERROR;
#else
        return send(sock, (char*)bytes, static_cast<int>(length), 0) != SOCKET_ERROR;
#endif
    }

    size_t SocketWinsock::receive(bool* success)
    {
#ifdef MS_PLATFORM_WASM
        int result = ws_recv(sock, (char*)buffer, MAX_PACKET_LENGTH, 0);
        if (result == WS_SOCK_ERROR)
        {
            *success = false;
            return 0;
        }
        return result;
#else
        timeval timeout = { 0, 0 };
        fd_set sockset = { 0 };
        FD_SET(sock, &sockset);
        int result = select(0, &sockset, 0, 0, &timeout);
        if (result > 0)
        {
            result = recv(sock, (char*)buffer, MAX_PACKET_LENGTH, 0);
        }
        if (result == SOCKET_ERROR)
        {
            *success = false;
            return 0;
        }
        else
        {
            return result;
        }
#endif
    }

    const int8_t* SocketWinsock::get_buffer() const
    {
        return buffer;
    }
}
#endif
