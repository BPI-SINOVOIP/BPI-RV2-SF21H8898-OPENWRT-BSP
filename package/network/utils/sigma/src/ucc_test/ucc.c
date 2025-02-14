#include <stdio.h>                                                                               
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

//#define SERVER_IP "192.168.1.1"
#define SERVER_IP "192.165.100.22"
#define SERVER_PORT 9000
#define TIMEOUT_SEC 5

int main() {
    int client_socket;
    struct sockaddr_in server_address;
    char message[1024];
    fd_set read_fds;
    struct timeval timeout;

    // 创建套接字
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 设置服务器地址结构
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);

    // 将IP地址从文本转换为二进制形式
    if (inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr) <= 0) {
        perror("invalid address / address not supported");
        exit(EXIT_FAILURE);
    }

    // 连接服务器
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("connection failed");
        exit(EXIT_FAILURE);
    }
    printf("[ucc]Connected to server\n");

    while (1) {
        // 发送数据
        printf("[ucc]Enter message to send (type 'exit' to quit): ");
        fgets(message, 1024, stdin);
        send(client_socket, message, strlen(message), 0);

        if (strncmp(message, "exit", 4) == 0) {
            break;
        }

        while (1) {
            // 清空文件描述符集合
            FD_ZERO(&read_fds);
            FD_SET(client_socket, &read_fds);

            // 设置超时时间
            timeout.tv_sec = TIMEOUT_SEC;
            timeout.tv_usec = 0;

            // 监视套接字是否可读
            int result = select(client_socket + 1, &read_fds, NULL, NULL, &timeout);
            if (result == -1) {
                perror("select failed");
                exit(EXIT_FAILURE);
            } else if (result == 0) {
                //printf("[ucc]Timeout occurred\n");
                break;
            }

            if (FD_ISSET(client_socket, &read_fds)) {
                // 接收数据
                memset(message, 0, sizeof(message));
                ssize_t bytes_received = recv(client_socket, message, sizeof(message) - 1, 0);
                if (bytes_received == -1) {
                    perror("recv failed");
                    exit(EXIT_FAILURE);
                } else if (bytes_received == 0) {
                    printf("[ucc]Connection closed by server\n");
                    break;
                }

                printf("[ucc]Received: %s", message);
            }
        }
        // 接收数据
        //memset(message, 0, sizeof(message));
        //recv(client_socket, message, 1024, 0);
        //printf("[ucc]Received: %s", message);
    }

    // 关闭连接
    close(client_socket);
    return 0;
}
