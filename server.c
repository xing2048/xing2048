#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <ctype.h>

#define PORT 8888
#define LOG_FILE "server.log"
#define TIMEOUT 45

void log_event(const char *msg) {
    FILE *fp = fopen(LOG_FILE, "a");
    if (fp) {
        time_t now;
        time(&now);
        fprintf(fp, "%.24s: %s\n", ctime(&now), msg);
        fclose(fp);
    }
}

int is_digits(const char *s) {
    while (*s) if (!isdigit(*s++)) return 0;
    return 1;
}

int is_alpha(const char *s) {
    while (*s) if (!isalpha(*s++)) return 0;
    return 1;
}

int has_chinese(const char *s) {
    while (*s) if ((unsigned char)*s++ > 0x7F) return 1;
    return 0;
}

int main() {
    log_event("服务端启动");
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 5);

    while (1) {
        usleep(500000);
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(server_fd, &fds);
        
        if (select(server_fd+1, &fds, NULL, NULL, &(struct timeval){0}) <= 0)
            continue;

        struct sockaddr_in cli_addr;
        socklen_t len = sizeof(cli_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&cli_addr, &len);
        
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &cli_addr.sin_addr, ip, sizeof(ip));
        char log_msg[100];
        sprintf(log_msg, "连接来自: %s", ip);
        log_event(log_msg);

        struct timeval tv = {TIMEOUT, 0};
        setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        char buf[1024];
        int n = recv(client_fd, buf, sizeof(buf)-1, 0);
        if (n > 0) {
            buf[n] = '\0';
            if (strcmp(buf, "stop") == 0 || strcmp(buf, "停服") == 0) {
                log_event("收到停止指令，服务端关闭");
                close(client_fd);
                close(server_fd);
                exit(0);
            }
            
            const char *resp;
            if (is_digits(buf))        resp = "数数";
            else if (is_alpha(buf))   resp = "ABC";
            else if (has_chinese(buf))resp = "文文";
            else                      resp = "未知类型";
            
            send(client_fd, resp, strlen(resp), 0);
        }
        
        close(client_fd);
        log_event("连接关闭");
    }

    close(server_fd);
    return 0;
}