#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <map>
#include <sstream>




std::map<std::string, std::string> parseHttpRequest(const std::string& httpRequest);
std::string parseReqPath(std::string path);

int main(int argc, char **argv) {

  std::cout << "Logs from your program will appear here!\n";


  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }

  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }

  //set port to 4221
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);

  //bind to port
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }

  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }

  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);

  std::cout << "Waiting for a client to connect...\n";

  //buffer to read from socket
  char msgBuffer[1024] = {0};
  //OK response
  //hardcoded, leave me alone
  std::string okMessage = "HTTP/1.1 200 OK\r\n\r\n";
  std::string notFound = "HTTP/1.1 404 Not Found\r\n\r\n";


  //stored the socket
  int clientSocket = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
  //received the message to buffer
  recv(clientSocket, msgBuffer, sizeof(msgBuffer), 0);
  
  //convert the buffer from c-style string to std::string
  std::string str(msgBuffer);

  //only for testing purposes, print out the parsed request
  std::cout << "Received message: " << std::endl;
  std::map<std::string, std::string> parsedRequest = parseHttpRequest(msgBuffer);
    // Print the parsed request
    for (const auto& pair : parsedRequest) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }
  
  //the parsed path (the path, excluding the first segment of it)
  std::string parsedPath = parseReqPath(parsedRequest["Path"]);
  //the length of the response
  std::string contentLength = "Content-Length: " + std::to_string(parsedPath.length());


    
  //routing
  //some arbitrary stuff, for example anything that start with /echo will
  //give you back the path that comes after the echo command
  //hone only sends ok
  if(parsedRequest["Path"].compare("/") == 0) {
      send(clientSocket, okMessage.c_str(), okMessage.length(), 0);
      std::cout << "Path found, sent 200" << std::endl;
  }

  //any part starting with /echo will receive a full body response
  else if(parsedRequest["Path"].substr(0, 5) == "/echo") {
      //this should not repeat, should pull into a function later
      std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n" + 
            contentLength + "\r\n\r\n" + parsedPath;

      send(clientSocket, response.c_str(), response.length(), 0);
      std::cout << "Path found, sent full response" << std::endl;
  }

  else if(parsedRequest["Path"] == "/user-agent") {
      std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n" + 
            contentLength + "\r\n\r\n" + parsedRequest["User-Agent"];

      response = response.c_str();
      send(clientSocket, response.c_str(), response.length() + 9, 0);
      std::cout << "Path found, sending user agent" << std::endl;
  }

  //if not in any of the above cases, send not found
  else {
      send(clientSocket, notFound.c_str(), notFound.length(), 0);
      std::cout << "Path not found, sent 404" << std::endl;
  }

  close(server_fd);

  return 0;
}


//parse the path(eliminate the first segment of it)
std::string parseReqPath(std::string path) {
  //grab everything after the second slash  

  size_t firstSlash = path.find('/');

  if (firstSlash != std::string::npos && firstSlash != path.length() - 1) {
    size_t secondSlash = path.find('/', firstSlash + 1);

    if (secondSlash != std::string::npos) {
      // Return the substring starting from the character after the second forward slash
      return path.substr(secondSlash + 1);
    }

  }
  return path;
}


std::map<std::string, std::string> parseHttpRequest(const std::string& request) {

    std::map<std::string, std::string> headerMap;

    size_t startPos = 0;
    size_t endPos = request.find("\r\n", startPos);

    while (endPos != std::string::npos) {
        std::string line = request.substr(startPos, endPos - startPos);

        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            
            //trim
            value.erase(0, value.find_first_not_of(" \t\r\n"));

            // Insert into map
            headerMap[key] = value;
        }

        startPos = endPos + 2; // Skip "\r\n"
        endPos = request.find("\r\n", startPos);
    }

    // Extract HTTP method and path from the first line
    size_t firstLineEndPos = request.find("\r\n");
    if (firstLineEndPos != std::string::npos) {
        std::string firstLine = request.substr(0, firstLineEndPos);
        size_t firstSpacePos = firstLine.find(' ');
        if (firstSpacePos != std::string::npos) {
            std::string method = firstLine.substr(0, firstSpacePos);
            std::string path = firstLine.substr(firstSpacePos + 1);
            char *pathFinal = strtok(path.data(), " ");
            headerMap["Method"] = method;
            headerMap["Path"] = pathFinal;
        }
    }

    return headerMap;
}