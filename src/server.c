#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <time.h>
#include <signal.h>

#include "vtr.h"
#include "cmd_proc.h"
#include "dlist.h"
#include "g_data.h"

#define container_of(ptr, T, member) \
    ((T *)( (char *)ptr - offsetof(T, member) ))

size_t run = 1;
void handle_sigint(){
    run = 0;
}

struct g_data gd;

// -------------------------- Utility Function -----------------------------

static void fd_to_nb(int fd){
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// -------------------------- Request Handling Function -----------------------------

void handle_request(struct Conn* conn){
    size_t offset = 0;
    uint32_t nstr;
    bufcpylen(conn->rbuf, &nstr, HEADER_SIZE);
    offset += HEADER_SIZE;

    char* cmd = get_string(conn->rbuf, &offset);

    int good_request = 0;
    for (size_t i=0; i<command_list_len; ++i){
        if (strcmp(cmd, command_list[i].cmd) == 0 && nstr == command_list[i].nstr){
            good_request = 1;
            int rv = command_list[i].handler(conn, offset);
            if (rv == 0){
                msg("handle_request (server.c): buffer size limit exceeded");
                conn->want_close = 1;
            }
        }
    }
    free(cmd);

    if (good_request == 0){
        int rv = response_code(conn->wbuf, RES_ERR);
        rv &= response_u32(conn->wbuf, 0);
        if (rv == 0){
            msg("handle_request (server.c): buffer size limit exceeded");
            conn->want_close = 1;
        }
    }
}

int try_one_request(struct Conn* conn){
    size_t sz = bufsize(conn->rbuf);
    if (sz < 4){
        return 0; // nstr not present
    }

    size_t offset = 0;
    uint32_t nstr = 0;
    bufcpylen(conn->rbuf, &nstr, HEADER_SIZE);
    offset += HEADER_SIZE;

    for (uint32_t i = 0; i < nstr; ++i){
        if (sz < 4 + offset){
            return 0; // len not present
        }
        uint32_t len;
        bufcpylenoffset(conn->rbuf, offset, &len, HEADER_SIZE);

        if (len > (uint32_t)MAX_PAYLOAD_SIZE){
            msg("try_one_request (server.c): Too long message");
            conn->want_close = 1;
            return 0;
        }
        offset += HEADER_SIZE;
        if (sz < len + offset){
            return 0; // str not present
        }
        offset += len;
    }

    handle_request(conn);
    bufconsume(conn->rbuf, offset);
    return 1;
}

// -------------------------- Handling Function -----------------------------
struct Conn* handle_accept(int fd){
    struct Conn* conn = create_conn(0);
    if (conn == NULL){
        return NULL;
    }

    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int cfd = accept(fd, (struct sockaddr *)& addr, &len);
    if (cfd < 0){
        msg("handle_accept (server.c): accept");
        free_conn(conn);
        return NULL;
    }

    fd_to_nb(cfd);

    conn->fd = cfd;
    return conn;
}

int handle_read(struct Conn* conn){
    if (bufmakespacelen(conn->rbuf, READ_CHUNK) == 0){
        msg("handle_read (server.c): bufmakespacelen");
        return 0;
    }

    ssize_t rv = read(conn->fd, bufend(conn->rbuf), bufavail(conn->rbuf));
    if (rv <= 0){
        if (rv < 0 && errno == EAGAIN){
            return 0;
        }
        conn->want_close = 1;
        return 0;
    }
    else {
        conn->rbuf->size += rv;
    }

    while (try_one_request(conn));
    
    if (bufsize(conn->wbuf) > 0){
        return 1;
    }
    return 0;
}

int handle_write(struct Conn* conn){
    ssize_t rv = write(conn->fd, bufstart(conn->wbuf), bufsize(conn->wbuf));
    if (rv < 0 && errno == EAGAIN){
        return 0;
    }
    if (rv <= 0){
        conn->want_close = 1;
        return 0;
    }

    bufconsume(conn->wbuf, rv);
    if (bufsize(conn->wbuf) == 0) return 1;
    else return 0;
}

// -------------------------- Main Server -----------------------------

int main(int argc, char* argv[]){

    if (argc > 1) {
        if (load_config(argv[1]) == 0){
            printf("[NOTE] | Successfully opened config file\n");
        }
    }

    if (log_open(logfile_path) != 0) {
        printf("[ERROR] | Failed to open log file, using stderr\n");
    }

    note("Starting Server - Welcome to Ramora");
    signal(SIGINT, handle_sigint);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("main (server.c): socket");
        log_close();
        return 0;
    }

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    int sz = 1<<15;
    setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sz, sizeof(sz));
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &sz, sizeof(sz));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, bind_addr, &(addr.sin_addr)) != 1){
        die("Invalid host");
        log_close();
        return 0;
    }
    int rv = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (rv){
        die("main (server.c): bind");
        close(fd);
        log_close();
        return 0;
    }
    fd_to_nb(fd);
    rv = listen(fd, SOMAXCONN);
    if (rv){
        die("main (server.c): listen");
        close(fd);
        log_close();
        return 0;
    }
    printf("[NOTE] | Listening on \"%s\":%d\n", bind_addr, port);
    note("Listening...");

    struct vector fd2conn;
    if (init_vector(&fd2conn, 1) == 0){
        die("main (server.c): init_vector");
        close(fd);
        log_close();
        return 0;
    }
    
    int epfd = epoll_create1(0);
    if (epfd < 0){
        die("main (server.c): epoll_create1");
        close(fd);
        free_vector(&fd2conn);
        log_close();
        return 0;
    }

    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);

    struct epoll_event events[MAX_EVENTS];

    if (init_g_data(&gd) == 0){
        die("main (server.c): init_g_data");
        close(fd);
        close(epfd);
        free_vector(&fd2conn);
        log_close();
        return 0;
    }

    while (run){
        int timeout_ms = smallest_remaining_time();
        int n_fd = epoll_wait(epfd, events, MAX_EVENTS, timeout_ms);
        int close_now = 0;
        for (int i = 0; i < n_fd; i++){
            int cfd = events[i].data.fd;
            uint32_t ev_flags = events[i].events;

            if (cfd == fd){
                struct Conn* conn = handle_accept(fd);
                if (conn){
                    struct epoll_event client_ev;
                    client_ev.data.fd = conn->fd;
                    client_ev.events = EPOLLIN;
                    epoll_ctl(epfd, EPOLL_CTL_ADD, conn->fd, &client_ev);

                    while ((int)fd2conn.cap <= conn->fd){
                        if (resize_vector(&fd2conn) == 0){
                            die("main (server.c): resize_vector");
                            close_now = 1;
                            break;
                        }
                    }
                    fd2conn.data[conn->fd] = conn;
                }
            }
            else if ((ev_flags & (EPOLLERR | EPOLLHUP)) || ((struct Conn *)(fd2conn.data[cfd]))->want_close == 1){
                close(cfd);
                free_conn(fd2conn.data[cfd]);
                fd2conn.data[cfd] = NULL;
                epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, 0);
            }
            else {
                if (ev_flags & EPOLLIN){
                    if (handle_read(fd2conn.data[cfd]) == 1){
                        struct epoll_event cev;
                        cev.data.fd = cfd;
                        cev.events = EPOLLOUT;
                        epoll_ctl(epfd, EPOLL_CTL_MOD, cfd, &cev);
                    }
                    conn_update_timer(fd2conn.data[cfd]);
                }
                if (ev_flags & EPOLLOUT){
                    if (handle_write(fd2conn.data[cfd]) == 1){
                        struct epoll_event cev;
                        cev.data.fd = cfd;
                        cev.events = EPOLLIN;
                        epoll_ctl(epfd, EPOLL_CTL_MOD, cfd, &cev);
                    }
                    conn_update_timer(fd2conn.data[cfd]);
                }
            }

            if (close_now) break;
        }

        {
            uint64_t time_of_expiration = get_monotonic_msec() - ((uint64_t)IDLE_TIMEOUT_MS);
            while (!dlist_empty(&gd.idle_list)){
                struct Conn* conn = (struct Conn*)container_of(gd.idle_list.next, struct Conn, idle_node);
                if (conn->last_used_time > time_of_expiration){
                    break;
                }
                close(conn->fd);
                fd2conn.data[conn->fd] = NULL;
                epoll_ctl(epfd, EPOLL_CTL_DEL, conn->fd, 0);
                free_conn(conn);
            }

            int max_expiration_count = 10;
            int expired_count = 0;
            uint64_t now_ms = get_monotonic_msec();
            while (expired_count < max_expiration_count && gd.ttl_heap.sz > 0 && gd.ttl_heap.arr[0].expiration_time <= now_ms){
                del_Entry(container_of(gd.ttl_heap.arr[0].ref, struct Entry, heap_idx)->key->data);
                expired_count++;
            }
        }
    }

    note("Freeing Hash Map");
    free_HMap(&gd.kv_db);

    note("Freeing Pooled Conn Objects");
    while (!dlist_empty(&gd.conn_pool)){
        struct Conn* conn = (struct Conn*)container_of(gd.conn_pool.next, struct Conn, idle_node);
        dlist_delete(&conn->idle_node);
        hard_free_conn(conn);
    }

    note("Freeing TTL Heap");
    heap_free(&gd.ttl_heap);

    note("Freeing allocated memory");
    for (size_t i=0; i<fd2conn.sz; ++i){
        if (fd2conn.data[i]){
            close(i);
            hard_free_conn(fd2conn.data[i]);
            fd2conn.data[i] = NULL;
            epoll_ctl(epfd, EPOLL_CTL_DEL, i, 0);
        }
    }
    free_vector(&fd2conn);

    note("Closing Endpoints");
    close(epfd);

    close(fd);
    note("Done, Exiting");
    log_close();
}