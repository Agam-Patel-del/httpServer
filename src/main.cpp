#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sstream>
#include <vector>
#include <thread>
#include <atomic>
#include <netinet/in.h>
#include <map>

int main(int argc, char **argv){
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if(serverFd < 0){
        std::cerr << "Failed to create server socket\n";
        return 1;
    }

    int reuse = 1;
    if(setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0){
        std::cerr << "setsockopt failed\n";
        close(serverFd);
        return 1;
    }
    
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET; // use IPV4 aaddress
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(4221);

    if(bind(serverFd, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) != 0){
        std::cerr << "Failed to bind to port 4221\n";
        close(serverFd);
        return 1;
    }

    if(listen(serverFd, 5) != 0){
        std::cerr << "listen failed\n";
        close(serverFd);
        return 1;
    }

    std::atomic<bool> stopServer(false);

    std::thread inputThread([&](){
        int x=1;
        while(std::cin>>x){
            if(x==0){
                stopServer.store(true);
                ::shutdown(serverFd, SHUT_RDWR);
                break;
            }
        }
    });

    std::cout << "Waiting for a client to connect\n";
    while(!stopServer.load()){
        struct sockaddr_in clientAddr{};
        socklen_t clientAddrLen = sizeof(clientAddr);


        int clientFd = accept(serverFd, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if(clientFd < 0){
            if(stopServer.load()) break;
            std::cout << "Failed to accept the connection\n";
            continue;
        }

        char buffer[2048];
        int bytesReceived = recv(clientFd, buffer, sizeof(buffer)-1, 0);
        if(bytesReceived <= 0){
            close(clientFd);
            continue;
        }

        buffer[bytesReceived]='\0';

        std::stringstream request(buffer);
        std::string requestLine;
        std::getline(request, requestLine);
        std::istringstream requestLineStream(requestLine);
        std::string method;
        std::string path;
        std::string version;
        requestLineStream >> method >> path >> version;

        std::string headerLine;
        std::map<std::string,std::string> headers;
        while(std::getline(request, headerLine)){
            if(!headerLine.empty() && headerLine.back() == '\r') headerLine.pop_back();
            if(headerLine.empty()) break;

            int colon = headerLine.find(':');
            if(colon == std::string::npos) continue;

            std::string headerName = headerLine.substr(0, colon), headerValue="";
            std::transform(headerName.begin(), headerName.end(), headerName.begin(),[](unsigned char character){
                return std::tolower(character);
            });

            if(headerName.size() > 0){
                int valueStart = headerLine.find_first_not_of(" \t", colon + 1);
                if(valueStart != std::string::npos){
                    int valueEnd = headerLine.find_last_not_of(" \t\r");
                    headerValue = headerLine.substr(valueStart, valueEnd - valueStart + 1);
                }
                headers[headerName] = headerValue;
            }
        }

        std::string response;
        if(method.empty() || path.empty() || version.empty()){
            response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
        }
        else if(path == "/"){
            response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
        }
        else if(path == "/echo/abc"){
            response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 3\r\n\r\nabc";
        }
        else if(path == "/user-agent"){
            std::string userAgent = headers["user-agent"];
            response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " 
            + std::to_string(userAgent.size()) + "\r\n\r\n" + userAgent;
        }
        else{
            response = "HTTP/1.1 404 Not Found\r\n\r\n";
        }

        std::cout << "Client connected\n";
        send(clientFd, response.c_str(), response.size(), 0);
        close(clientFd);
    }

    inputThread.join();
    close(serverFd);
    return 0;
}
