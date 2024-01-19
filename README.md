# How to Find Your Raspberry Pi Address When Connected to a DHCP Network

In environments where network devices receive dynamic IP addresses via DHCP (Dynamic Host Configuration Protocol), locating a specific device like a Raspberry Pi can be a challenge. This is particularly true in scenarios where you don't have access to the network router or admin interface to check connected devices. One practical solution is to set up a UDP broadcast system consisting of a client and a server, allowing your Raspberry Pi to respond with its IP address when queried. In this article, we'll walk through creating such a system using C++.

## The Concept: UDP Broadcast System

Our solution involves two components: a UDP server running on the Raspberry Pi and a UDP client that can run on any computer in the same network. The client sends a broadcast message to the network, and the server, upon receiving this message, responds with its IP address.

## Building the UDP Server for Raspberry Pi

The server program is designed to run on the Raspberry Pi. It continuously listens for incoming UDP messages. When it receives a specific message (in our case, "wheremypi"), it responds with the IP address of the Pi's `eth0` network interface.

Here's the server program in C++:

```cpp
// Include necessary headers
#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <chrono>

// Function to get the IP Address of the Raspberry Pi
std::string getIPAddress() {
    struct ifaddrs *ifaddr, *ifa;
    std::string ipAddress = "";

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
            if (strcmp(ifa->ifa_name, "eth0") == 0) {
                ipAddress = inet_ntoa(((struct sockaddr_in *)ifa->ifa_addr)->sin_addr);
                break;
            }
        }
    }

    freeifaddrs(ifaddr);
    return ipAddress;
}

int main() {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
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
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    while (true) {
        char buffer[1024];
        socklen_t len = sizeof(cliaddr);
        auto start = std::chrono::high_resolution_clock::now();
        int n = recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr *)&cliaddr, &len);
        buffer[n] = '\0';

        if (strcmp(buffer, "wheremypi") == 0) {
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
```

This server listens for UDP packets on port 8080. When it receives a packet containing the string "wheremypi," it retrieves its own IP address and sends it back to the requester. It also logs the request with the client's IP address and the response time.

### Creating the UDP Client
The client program can be executed on any computer in the same network as the Raspberry Pi. It sends a UDP broadcast packet with the message "wheremypi" and waits for a response from the server(s) on the network.

Here's the client program in C++:

```cpp
// Include necessary headers
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    char buffer[1024];

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set options for broadcasting
    int broadcast = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
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
    const char *message = "wheremypi";
    sendto(sockfd, message, strlen(message), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
    std::cout << "Broadcast message sent.\n";

    // Receive response
    socklen_t len = sizeof(cliaddr);
    int n = recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr *)&cliaddr, &len);
    buffer[n] = '\0';
    std::cout << "Response from server: " << buffer << std::endl;

    close(sockfd);
    return 0;
}
```

This client sends a broadcast UDP packet to the network. Any server listening for the "wheremypi" message will respond with its IP address, which the client receives and prints out.

## Implementing the System

To implement this system:

1.  Write the Server and Client Programs: Use the provided C++ code to create the server and client programs.
2.  Compile the Programs: Compile both programs using a C++ compiler. Ensure the necessary libraries for network programming are installed.
3.  Run the Server on the Raspberry Pi: Start the server program on your Raspberry Pi. It will listen for the "wheremypi" message.
4.  Run the Client on Another Device: Execute the client program on another device in the same network. It will send the broadcast message and wait for a response.

## Creating a System Service for Continuous Server Operation

To ensure the Raspberry Pi's server script is always running and ready to respond to broadcast messages, we can create a system service. This involves setting up a service unit file for systemd, the init system used by most modern Linux distributions, including Raspberry Pi OS. The systemd service will manage the execution of our server script, ensuring it starts automatically at boot and restarts in case of failure.

Here's how to set up the server script as a systemd service:

1.  Compile the Server Program:

    First, compile the server program as an executable. For example, compile it to `/usr/local/bin/mypi-server`.

2.  Create a systemd Service File:

    Create a new file in `/etc/systemd/system/`, for example, `mypi-server.service`. Open the file with a text editor and add the following configuration:

    ```ini
    [Unit]
    Description=MyPi Server Service
    After=network.target

    [Service]
    ExecStart=/usr/local/bin/mypi-server
    Restart=on-failure
    User=pi
    Group=pi

    [Install]
    WantedBy=multi-user.target
    ```

    This service file tells systemd to start the `mypi-server` executable after the network is available. It will run under the `pi` user and group. If the server fails, systemd will attempt to restart it.

3. Enable and Start the Service:

    Enable the service to start on boot and start it immediately with the following commands:

    ```bash
    sudo systemctl daemon-reload
    sudo systemctl enable mypi-server.service
    sudo systemctl start mypi-server.service
    ```

4. Verify the Service is Running:

    Check the status of the service to ensure it's running correctly:

    ```bash
    sudo systemctl status mypi-server.service
    ```

5. View Logs:

    To view logs for your service, use the command:

    ```bash
    journalctl -u mypi-server.service
    ```

    By setting up the server program as a systemd service, you ensure that your Raspberry Pi is always listening for the "wheremypi" broadcast message. This setup is particularly useful for headless Raspberry Pi setups or when the Pi is used in embedded systems or IoT applications where constant network discoverability is crucial.

## Conclusion

This UDP broadcast system offers a straightforward and efficient method to discover the IP address of a Raspberry Pi on a DHCP network. By deploying this approach, you can easily locate your Pi without needing to access the network router or use additional network scanning tools, saving time and simplifying network management tasks.
