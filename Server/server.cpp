#include <iostream>
#include <stdlib.h>
#include <bits/stdc++.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <bits/stdc++.h>
#include <fstream>
#include <ctype.h>
#include <thread>
#include <pthread.h>
#include <errno.h>
#include "../Utilities/utilities.h"

using namespace std;

#define MAX_CLIENTS 50
#define BUFFER_SIZE 8192
// GLobals
atomic<int> current_number_of_clients(0); // prevent race conditions

int getContentLength(string request)
{
    int ans = 0;
    int startOfContent = request.find("Content-Length: ");
    if (startOfContent == -1)
        return 0;
    startOfContent += 16;
    while (isdigit(request[startOfContent]))
    {
        ans *= 10;
        ans += request[startOfContent] - '0';
        startOfContent++;
    }
    return ans;
}

string createResponse(string type, string body)
{
    stringstream header(type);
    stringstream response;
    response << "HTTP/1.1 ";

    string requestType, path;
    header >> requestType;
    header >> path;

    if (requestType == "GET")
    {
        ifstream fr;
        fr.open(path.substr(1));
        // File doesn't exist
        if (fr.is_open() == false)
        {
            response << "404 NOT FOUND\r\n\r\n";
        }
        else
        {
            response << "200 OK\r\n";
            pair<char *, int> fileInfo = getFileInfo(path.substr(1));
            response << "Content-Length: " << fileInfo.second << "\r\n\r\n";
            response.write(fileInfo.first, fileInfo.second);
            response << "\r\n";
            delete fileInfo.first; // clear heap
            fr.close();
        }
    }
    else
    { // POST
        response << "200 OK\r\n\r\n";
        writeFile(path.substr(1), body); // to remove '/' in the begining
    }
    return response.str();
}

void serve(int client_socket)
{
    char bufferReader[BUFFER_SIZE];
    int sent_bytes;

    // Keep serving till timeout
    while (true)
    {
        sent_bytes = recv(client_socket, bufferReader, BUFFER_SIZE, 0);

        if (sent_bytes == 0 || errno == EAGAIN || errno == EWOULDBLOCK)
        {
            cout << "Serving client failed" << endl;
            cout << "Closing connection" << endl;
            break;
        }

        string allRequest = bufToString(bufferReader, sent_bytes);
        string type = getType(allRequest);
        int contentLen = getContentLength(allRequest);

        string body = "";
        if (contentLen != 0)
            body = getBody(allRequest);

        int currLen = body.size();

        // Read body
        while (currLen < contentLen)
        {
            sent_bytes = recv(client_socket, bufferReader, BUFFER_SIZE, 0);
            currLen += sent_bytes;
            body += bufToString(bufferReader, sent_bytes);
        }

        cout << "All data has been recieved" << endl;
        // cout << type << endl << body <<endl;

        string response = createResponse(type, body);
        int total = response.size();
        int remaining = response.size();
        int sent = 0;
        while (sent < total)
        {
            ssize_t currSent = send(client_socket, response.c_str() + sent, remaining, 0);
            sent += currSent;
            remaining -= currSent;
        }
    }
    current_number_of_clients--;
    close(client_socket);
}

int main(int argc, char const *argv[])
{
    if (argc != 2)
    {
        cout << "Port Number not specified!!";
        return -1;
    }
    int portNum = atoi(argv[1]); // Listening port for socket

    // Socket creation
    int main_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (main_socket == -1)
    {
        cout << "Failed so create socket" << endl;
        exit(EXIT_FAILURE);
    }
    else
        cout << "Socket created successfully" << endl;

    // Initialize environment for socket address
    struct sockaddr_in server_address, client_address;
    int lenAddr = sizeof(client_address);
    server_address.sin_family = AF_INET;         // Address family for TCP
    server_address.sin_port = htons(portNum);    // Little endian to big endian
    server_address.sin_addr.s_addr = INADDR_ANY; // Address of local machine
    memset(server_address.sin_zero, 0, sizeof(server_address.sin_zero));

    // Binding
    int bindState = bind(main_socket, (struct sockaddr *)&server_address, sizeof(server_address));
    if (bindState < 0)
    {
        cout << "Failed to bind" << endl;
        return -1;
    }
    else
        cout << "Binding done successfully" << endl;

    // Listening
    int listenState = listen(main_socket, 10); // Max of 10 clients can be waiting in the queue
    if (listenState < 0)
    {
        cout << "Listening failed" << endl;
        return -1;
    }
    else
        cout << "Server started listening on port : " << portNum << endl;

    // Socket open and waiting for clients
    while (true)
    {
        cout << "Awaiting for clients....." << flush;

        int client_socket = accept(main_socket, (struct sockaddr *)&server_address, (socklen_t *)&lenAddr);
        if (client_socket < 0)
        {
            cout << "Failed to conenct to client" << endl;
            exit(EXIT_FAILURE);
        }

        char clientName[INET_ADDRSTRLEN]; // client address as string
        if (inet_ntop(AF_INET, &client_address.sin_addr.s_addr, clientName, sizeof(clientName)) == NULL)
            cout << "Failed to get client address as string" << endl;

        current_number_of_clients++;
        struct timeval timeout;
        timeout.tv_usec = ((float)MAX_CLIENTS / current_number_of_clients) * 1000000 + 500000; // timeout faster if number of clients increases
        timeout.tv_sec = 0;
        setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        thread client_request(serve, client_socket); // serve client request while taking other requests concurently
        client_request.detach();
    }

    close(main_socket);
    return 0;
}
