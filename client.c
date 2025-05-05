#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#define CONFIG_FILE "client.conf"
#define MAX_ATTEMPTS 3
#define CONNECT_TIMEOUT 4
#define RECV_TIMEOUT 3

int main() {
    char server_addr[100] = {0};
    FILE *config = fopen(CONFIG_FILE, "r");
    if (config) {
        fgets(server_addr, sizeof(server_addr), config);
        fclose(config);
        server_addr[strcspn(server_addr, "\n")] = '\0';
    } else {
        printf("没有client.conf配置文件\n请输入服务器地址：");
        fgets(server_addr, sizeof(server_addr), stdin);
        server_addr[strcspn(server_addr, "\n")] = '\0';
    }

    int sockfd, retries = 0;
    struct sockaddr_in serv_addr;

    while (retries < MAX_ATTEMPTS) {
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("socket创建失败");
            exit(EXIT_FAILURE);
        }

        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(8888);
        
        if (inet_pton(AF_INET, server_addr, &serv_addr.sin_addr) <= 0) {
            perror("地址无效");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        struct timeval timeout = {CONNECT_TIMEOUT, 0};
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == 0) {
            break;
        }

        close(sockfd);
        if (++retries < MAX_ATTEMPTS) sleep(1);
    }

    if (retries == MAX_ATTEMPTS) {
        printf("连接失败\n");
        exit(EXIT_FAILURE);
    }

    printf("输入要发送的内容：");
    char buffer[1024];
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
    send(sockfd, buffer, strlen(buffer), 0);

    struct timeval tv = {RECV_TIMEOUT, 0};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    char recv_buf[1024];
    int len = recv(sockfd, recv_buf, sizeof(recv_buf)-1, 0);
    if (len > 0) {
        recv_buf[len] = '\0';
        printf("服务器响应：%s\n", recv_buf);
    }

    close(sockfd);
    return 0;
}