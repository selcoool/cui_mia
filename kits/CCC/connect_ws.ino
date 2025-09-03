#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <iostream>
#include <ixwebsocket/IXWebSocket.h>

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed: " << iResult << std::endl;
        return 1;
    }
#endif

    ix::WebSocket webSocket;

    webSocket.setUrl("ws://localhost:3000/ws");

    webSocket.setOnMessageCallback([](const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Message) {
            std::cout << "Received: " << msg->str << std::endl;
        }
        });

    webSocket.start();

    std::cout << "Press Enter to quit..." << std::endl;
    std::cin.get();

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}
