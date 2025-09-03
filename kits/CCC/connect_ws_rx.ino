#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <iostream>
#include <thread>
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

    // callback chỉ để nhận log
    webSocket.setOnMessageCallback([](const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Open) {
            std::cout << "✅ Connected to server!" << std::endl;
        }
        else if (msg->type == ix::WebSocketMessageType::Message) {
            std::cout << "📩 Received: " << msg->str << std::endl;
        }
        else if (msg->type == ix::WebSocketMessageType::Error) {
            std::cerr << "❌ Error: " << msg->errorInfo.reason << std::endl;
        }
        });

    webSocket.start();

    // đợi kết nối xong chút
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // gửi request get_users
    webSocket.sendText(R"({"type":"get_users"})");

    // thread nhập liệu từ console
    std::thread sender([&webSocket]() {
        while (true) {
            std::string input;
            std::getline(std::cin, input);
            if (input == "exit") break;
            webSocket.sendText(input);
        }
        });

    sender.join();

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
