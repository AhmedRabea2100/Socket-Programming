#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fstream>
#include <bits/stdc++.h>
#include "../Utilities/utilities.h"

using namespace std;

#define BUFFER_SIZE 8192

string create_GET_request(string path)
{
    stringstream request;
    request << "GET ";
    request << path;
    request << " HTTP/1.1\r\n\r\n";
    return request.str();
}

int GET_response(int sockfd, string path)
{
    char buffer[BUFFER_SIZE];
    int readBytes = read(sockfd, buffer, BUFFER_SIZE);
    if (readBytes == -1)
    {
        printf("Error in reading\n");
        return 1;
    }
    string response = bufToString(buffer, readBytes);
    string header = getHeader(response);
    cout << header << endl;
    string body = getBody(response);

    char *p = strtok(buffer, "\r\n");
    int status;
    sscanf(p, "HTTP/1.1 %d", &status);

    if (status != 200)
    {
        return 0;
    }

    p = strtok(NULL, "\r\n");
    int contentLen;
    sscanf(p, "Content-Length: %d", &contentLen);

    while (body.size() < contentLen + 2)
    {
        int readBytes = read(sockfd, buffer, BUFFER_SIZE);
        if (readBytes == -1)
        {
            printf("Error in reading\n");
            return 1;
        }
        body += bufToString(buffer, readBytes);
    }
    writeFile(path.substr(1), body); // to remove '/' in the begining
    return 0;
}

string create_POST_request(string path)
{
    stringstream request;
    request << "POST ";
    request << path;
    request << " HTTP/1.1\r\n";
    pair<char *, int> fileInfo = getFileInfo(path.substr(1)); // to remove '/' in the begining
    request << "Content-Length: " << fileInfo.second << "\r\n\r\n";
    request.write(fileInfo.first, fileInfo.second);
    request << "\r\n";
    delete fileInfo.first; // clear heap
    return request.str();
}

int POST_response(int sockfd, string path)
{
    char buffer[BUFFER_SIZE];
    int readBytes = read(sockfd, buffer, BUFFER_SIZE);
    if (readBytes == -1)
    {
        printf("Error in reading\n");
        return 1;
    }
    string response = bufToString(buffer, readBytes);
    string header = getHeader(response);
    cout << header << endl;
    return 0;
}

int main(int argc, char const *argv[])
{
    if (argc != 3)
    {
        printf("Invalid arguments\n");
        return 1;
    }
    const char *ip = argv[1];
    int portNumber = atoi(argv[2]);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0); // IPv4, TCP
    if (sockfd == -1)
    {
        printf("Error in creating a socket\n");
        return 1;
    }

    struct sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portNumber);
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        printf("Error in connection\n");
        return 1;
    }

    FILE *fp = fopen("commands.txt", "r");
    if (fp == NULL)
    {
        printf("Commands not found\n");
        return 1;
    }

    char *line = NULL;
    size_t len = 0;
    while ((getline(&line, &len, fp)) != -1)
    {
        char *requestType = strtok(line, " ");
        char *p = strtok(NULL, "");
        string path = string(p);
        path.pop_back(); // remove \n
        if (strcmp(requestType, "client_get") == 0)
        {
            string response = create_GET_request(path);
            if (write(sockfd, response.c_str(), response.size()) == -1)
            {
                printf("Error in writing\n");
                return 1;
            }
            if (GET_response(sockfd, path))
                return 1;
        }
        else if (strcmp(requestType, "client_post") == 0)
        {
            string response = create_POST_request(path);
            int total = response.size();
            int remaining = total;
            int sent = 0;
            while (sent < total)
            {
                int writtenBytes = write(sockfd, response.c_str() + sent, remaining);
                sent += writtenBytes;
                remaining -= writtenBytes;
            }
            if (POST_response(sockfd, path))
                return 1;
        }
    }
    fclose(fp);
    close(sockfd);
    return 0;
}
