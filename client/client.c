#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <assert.h>
#include <signal.h>

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 5000

void handle_sigpipe(){
    printf("[ERROR]: Server closed unexpecteadly\n");
    exit(EXIT_FAILURE);
}

int write_full(int fd, const char *buf, size_t len) {
    while (len > 0) {
        ssize_t rv = write(fd, buf, len);
        if (rv < 0) {
            printf("[ERROR]: Server closed unexpecteadly\n");
            return -1;
        }
        buf += rv;
        len -= rv;
    }
    return 0;
}

int read_full(int fd, char *buf, size_t len) {
    while (len > 0) {
        ssize_t rv = read(fd, buf, len);
        if (rv < 0) {
            printf("[ERROR]: Server closed unexpecteadly\n");
            return -1;
        }
        buf += rv;
        len -= rv;
    }
    return 0;
}

enum ResCode {
    RES_OK = 0, // command executed successfully
    RES_ERR = 1, // error occured
    RES_NX = 2, // no entry found
    RES_TY = 3, // Type error
};

enum Tag {
    TAG_NULL = 0,
    TAG_INT = 1,
    TAG_STR = 2,
    TAG_ARR = 3,
    TAG_INT64 = 4,
};

int main(int argc, char* argv[]) {

    signal(SIGPIPE, handle_sigpipe);

    char host[64] = DEFAULT_HOST;
    int port = DEFAULT_PORT;

    for (int i = 1; i < argc; i++) {
        if ((!strcmp(argv[i], "-h") || !strcmp(argv[i], "--host")) && i + 1 < argc) {
            strncpy(host, argv[++i], sizeof(host) - 1);
        }
        else if ((!strcmp(argv[i], "-p") || !strcmp(argv[i], "--port")) && i + 1 < argc) {
            port = atoi(argv[++i]);
            if (port <= 0 || port > 65535){
                printf("[WARNING]: Incorrect port given, using the default port: \'5000\'\n");
                port = 5000;
            }
        }
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        printf("[ERROR]: couldn\'t create a socket\n");
        return 0;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &(addr.sin_addr)) != 1){
        printf("[ERROR]: Invalid host: %s\n", host);
    }

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        printf("[ERROR]: couldn\'t connect to server\n");
        return 0;
    }

    char line[1024];
    printf("Welcome to ramora client (type exit to quit)\n");

    while (1) {
        printf("> ");
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\n")] = 0;

        if (strcmp(line, "exit") == 0) break;

        char *argv[64];
        int argc = 0;
        char *tok = strtok(line, " ");
        while (tok && argc < 64) {
            argv[argc++] = tok;
            tok = strtok(NULL, " ");
        }

        if (argc == 0) continue;

        uint32_t nstr = argc;
        if (write_full(fd, (char *)&nstr, 4)) {
            printf("[ERROR]: write\n");
            close(fd);
            return 0;
        }

        // Send [len str] for each
        for (int i = 0; i < argc; i++) {
            uint32_t len = strlen(argv[i]);
            if (write_full(fd, (char *)&len, 4)) {
                printf("[ERROR]: write\n");
                close(fd);
                return 0;
            }
            if (write_full(fd, argv[i], len)) {
                printf("[ERROR]: write\n");
                close(fd);
                return 0;
            }
        }

        // Read response back: assume single string

        uint8_t res_code;
        read_full(fd, (char*)&res_code, 1);

        if (res_code == RES_OK) printf("code: RES_OK\n");
        else if (res_code == RES_NX) printf("code: RES_NX\n");
        else if (res_code == RES_TY) printf("code: RES_TY\n");
        else if (res_code == RES_ERR) printf("code: RES_ERR\n");
        else printf("code: RES_UNKNOWN\n");

        uint32_t res_count;
        read_full(fd, (char*)&res_count, 4);

        for (int i=0; i<(int)res_count; ++i){
            uint8_t tag;
            read_full(fd, (char *)&tag, 1);

            if (tag == TAG_NULL){
                printf("NULL\n");
            }
            else if (tag == TAG_INT){
                int a;
                read_full(fd, (char*)&a, 4);
                printf("INT: %d\n", a);
            }
            else if (tag == TAG_INT64){
                int64_t a;
                read_full(fd, (char*)&a, 8);
                printf("INT64: %ld\n", a);
            }
            else if (tag == TAG_STR){
                uint32_t len;
                read_full(fd, (char*)&len, 4);
                char* str = malloc(sizeof(char) * (len + 1));
                read_full(fd, str, len);
                str[len] = '\0';
                printf("STR: %s\n", str);
                free(str);
            }
            else {
                printf("UNKNOWN\n");
            }
        }
    }

    close(fd);
    return 0;
}
