#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sstream>
#include <vector>

int main(int argc, char **argv) {
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
    return 1;
  }
  
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET; // use IPV4 aaddress
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);
  
  if(bind(serverFd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0){
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }
  
  int connection_backlog = 5;
  if(listen(serverFd, connection_backlog) != 0){
    std::cerr << "listen failed\n";
    return 1;
  }
  
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  
  std::cout << "Waiting for a client to connect\n";
  
  int clientFd = accept(serverFd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
  if(clientFd < 0){
    std::cout<<"Failed to accept the connection\n";
  }

  char buffer[2048];
  int bytesRecieved = recv(clientFd, buffer, sizeof(buffer)-1, 0);
  buffer[bytesRecieved] = '\0';

  std::stringstream request(buffer);
  std::vector<std::string> args;
  std::string temp;
  while(request >> temp){
    args.push_back(temp);
  }

  std::string response;

  if(args[1] != "/"){
    response = "HTTP/1.1 400 Not Found\r\n\r\n";
  }
  else{
    response = "HTTP/1.1 200 OK\r\n\r\n";
  }

  std::cout << "Client connected\n";
  send(clientFd, response.c_str(), response.size(), 0);
  close(serverFd);

  return 0;
}
