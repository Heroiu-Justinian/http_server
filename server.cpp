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
#include <thread>

const std::string CRLF = "\r\n";
const std::string HTTP200 = "HTTP/1.1 200 OK" + CRLF;
const std::string HTTP404 = "HTTP/1.1 404 Not Found" + CRLF;


std::map<std::string, std::string> parse_http_request(const std::string& httpRequest);
std::string parse_request_path(std::string path);
std::string generate_basic_response(std::string response_body);
void handle_connection(int client_fd);

int main(int argc, char **argv) {

  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";

  // Uncomment this block to pass the first stage

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket" <<std::endl;;
   return 1;
  }

  // Since the tester restarts your program quite often, setting REUSE_PORT
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed" << std::endl;
    return 1;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);

  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 4221" << std::endl;
    return 1;
  }

  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed" <<std::endl;
    return 1;
  }

  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);

  std::cout << "Waiting for a client to connect...\n";

  
  while(true) {
 
    int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
    if(client_fd < 0) {
      std::cerr << "Failed to accept client connection" << std::endl;
      continue;
    }

    std::cout << "Client connected!" << std::endl;
    
    //fun fact, this runs so fast that even without the thread it still responds to roughly 70% of the requests
    //sent by oha lol =))

    std::thread t(handle_connection, client_fd);

    //join() would wait for each thread to be returned
    //which isn't useful since I want to take as many requests at once as possible
    t.detach();
  }

  close(server_fd);
 
  return 0;
}


void handle_connection(int client_fd) {

  /**
   * Given an int client_fd (client socket), handles the client connection
   * - Receives the incoming request to buffer
   * - Parses the request using parse_http_request()
   * - Sends correct message according to routing
   * - Closes connection when done
  */

  char request_buffer[1024] = {0};

  if (recv(client_fd, request_buffer, sizeof(request_buffer), 0) < 0) {
      throw std::runtime_error("Failed to receive the HTTP request, closing");
      close(client_fd);
    }
  //convert the buffer from c-style string to std::string
  std::string str(request_buffer);

  //only for testing purposes, print out the parsed request
  std::cout << "Received request: " << std::endl;
  std::map<std::string, std::string> parsed_request = parse_http_request(request_buffer);
    // Print the parsed request
  for (const auto& pair : parsed_request) {
      std::cout << pair.first << ": " << pair.second << std::endl;
  }


 //Routing
  std::string response;

  //hone only sends ok
  if(parsed_request["Path"].compare("/") == 0) {
      response = HTTP200 + CRLF;

      std::cout << "Path found, sending 200 OK" << std::endl;
  }

  //requests for paths that start with /echo will receive back everything after the echo
  else if(parsed_request["Path"].substr(0, 5) == "/echo") {
      std::string parsed_path = parse_request_path(parsed_request["Path"]);
      response  = generate_basic_response(parsed_path);

      std::cout << "Echoing the path efter /echo" << std::endl;
  }

  //requests that start with /user-agent will get back the user agent
  else if(parsed_request["Path"] == "/user-agent") {
      std::string user_agent = parsed_request["User-Agent"];
      response = generate_basic_response(user_agent);
      
      std::cout << "Sending back the user agent that made the request" << std::endl;
  }

  //if not in any of the above cases, send not found
  else {
      response = HTTP404 + CRLF;
      std:: cout << response << std::endl;

      std::cout << "Path not found, sending 404" << std::endl;
  }


  //send response
  //if error, log to the console
  if (send(client_fd, response.c_str(), response.length(), 0) == -1) {
    //not terminating the program if send is impossible, might be caused by client
    std::cerr << "There was an errr sending the HTTP response to client: " << std::to_string(client_fd) << std::endl;
  }

  close(client_fd);
}

std::string generate_basic_response(std::string response_body) {

  /**
   * Sends a basic response back to the client
   * Response only includes the basic header stuff (like the 200 OK, Content Type and Lenght)
   * and a response body (e.g. for the echo route it's the path after the /echo)
  */

  std::string content_len = "Content-Length: " + std::to_string(response_body.length());

  std::string response = HTTP200 + "Content-Type: text/plain" + CRLF + 
    content_len + CRLF;
  response = response + CRLF; //one more \r\n\ for end of header
  response = response + response_body;
  
  return response;
}


std::string parse_request_path(std::string path) {
  /**
   * Given a path, will return everything after the first segment
  */

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


std::map<std::string, std::string> parse_http_request(const std::string& request) {
    /**
     * Takes in a request as a std::string
     * Returns a std::map<std::string, std::string> with the parsed request
    */

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

            headerMap[key] = value;
        }

        startPos = endPos + 2; // Skip "\r\n"
        endPos = request.find("\r\n", startPos);
    }

    // Extract HTTP method and path from the first line manually
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