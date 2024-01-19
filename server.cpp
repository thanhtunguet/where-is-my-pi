#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <chrono>
#include <cstring>
#include "common.h"

std::string getIPAddress()
{
    struct ifaddrs *ifaddr, *ifa;
    std::string ipAddress = "";

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET)
        {
            if (strcmp(ifa->ifa_name, NET_INTERFACE) == 0)
            {
                ipAddress = inet_ntoa(((struct sockaddr_in *)ifa->ifa_addr)->sin_addr);
                break;
            }
        }
    }

    freeifaddrs(ifaddr);
    return ipAddress;
}

int main()
{
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(8080);

    // Bind socket to server address
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    while (true)
    {
        char buffer[1024];
        socklen_t len = sizeof(cliaddr);
        auto start = std::chrono::high_resolution_clock::now();
        int n = recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr *)&cliaddr, &len);
        buffer[n] = '\0';

        if (strcmp(buffer, MESSAGE) == 0)
        {
            std::string ipAddress = getIPAddress();
            sendto(sockfd, ipAddress.c_str(), ipAddress.size(), 0, (const struct sockaddr *)&cliaddr, len);

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end - start;
            std::cout << "Receive request from " << inet_ntoa(cliaddr.sin_addr)
                      << ": responded in " << elapsed.count() << " ms" << std::endl;
        }
    }

    close(sockfd);
    return 0;
}
