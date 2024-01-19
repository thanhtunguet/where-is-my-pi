#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "common.h"

int main()
{
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    char buffer[1024];

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set options for broadcasting
    int broadcast = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0)
    {
        perror("setsockopt (SO_BROADCAST)");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(8080);
    servaddr.sin_addr.s_addr = inet_addr("255.255.255.255");

    // Send message

    sendto(sockfd, MESSAGE, strlen(MESSAGE), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
    std::cout << "Broadcast message sent.\n";

    // Receive response
    socklen_t len = sizeof(cliaddr);
    int n = recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr *)&cliaddr, &len);
    buffer[n] = '\0';
    std::cout << "Response from server: " << buffer << std::endl;

    close(sockfd);
    return 0;
}
