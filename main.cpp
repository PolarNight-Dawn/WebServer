//
// Created by polarnight on 24-3-18, 上午12:09.
//

/* 处理I/O 读写 */

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <csignal>
#include <cassert>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <cerrno>
#include <unistd.h>

#include "locker.h"
#include "threadpool.h"
#include "http_conn.h"
#include "lst_timer.h"

#define MAX_FD 65536
#define MAX_EVENT_NUMBER 10000

/* 注册事件到 epoll 指定的内核事件表 */
extern void addfd(int epollfd, int fd, bool one_shot);

/* 注销 epoll 内核事件表上的事件 */
extern void removefd(int epollfd, int fd);

/* 修改 epoll 内核事件表上的事件 */
extern void modfd(int epollfd, int fd, int ev);

void addsig(int sig, void(*sig_handler)(int), bool restart = true) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = *sig_handler;
    if (restart) {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, nullptr) != -1);
}

void show_errno(int connfd, const char *info) {
    printf("%s", info);
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int main(int argc, char *argv[]) {
    if (argc <= 2) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        exit(1);
    }

    /* 获取 IP 地址 */
    const char *ip = argv[1];
    /* 获取 port */
    int port = atoi(argv[2]);

    /* 处理SIGPIPE信号 */
    addsig(SIGPIPE, SIG_IGN);

    /* 创建线程池 */
    threadpool<http_conn> *pool = nullptr;
    try {
        pool = new threadpool<http_conn>;
    } catch (...) {
        exit(1);
    }

    /* 预先为每一个客户连接分配一个 http_conn 对象*/
    http_conn *users = new http_conn[MAX_FD];
    assert(users);

    /* 创建 socket */
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= -1);

    /* 端口复用 */
    struct linger tmp = {1, 0};
    setsockopt(listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));

    /* 绑定 */
    int ret = 0;
    struct sockaddr_in address;
    memset(&address, '\0', sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr.s_addr);
    address.sin_port = htons(port);

    ret = bind(listenfd, (struct sockaddr *) &address, sizeof(address));
    assert(ret != 0);

    /* 监听 */
    ret = listen(listenfd, 5);
    assert(ret != -1);

    /* 创建 epoll 实例 */
    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    assert(epollfd != -1);
    addfd(epollfd, listenfd, false);
    http_conn::m_epollfd = epollfd;

    /* 创建管道套接字 */
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
    assert(ret != -1);

    /* 写端设置非阻塞 */
    setnonblocking(pipefd[1]);

    /* 设置管道读端为 ET 非阻塞 */
    addfd(epollfd, pipefd[0], false);

    /* 传递给主循环的信号 */
    addsig(SIGALRM, sig_handler, false);
    addsig(SIGTERM, sig_handler, false);

    /* 循环条件 */
    bool stop_server = false;

    /* 超时标志 */
    bool timeout = false;

    /* 每隔TIMESLOT 时间触发SIGALRM信号 */
    alarm(TIMESLOT);

    while (true) {
        /* 就绪事件的数量 */
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if ((number < 0) && (errno != EINTR)) {
            printf("epoll failure\n");
            break;
        }

        for (int i = 0; i < number; i++) {
            int sockfd = events[i].data.fd;
            if (sockfd == listenfd) {
                /* 检测到有客户连接 */
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);

#ifdef LT
                int connfd = accept(listenfd, (struct sockaddr*) &client_address, &client_addrlength);
                if (connfd < 0) {
                    printf("errno is： %d\n", errno);
                    continue;
                }
                if (http_conn::m_user_count >= MAX_FD) {
                    show_errno(connfd, "Internal server busy");
                    continue;
                }
                /* 初始化客户连接 */
                users[connfd].init(connfd, client_address);
#endif

#ifdef ET
                while (true) {
                    int connfd = accept(listenfd, (struct sockaddr *) &client_address, &client_addrlength);
                    if (connfd < 0) {
                        printf("errno is： %d\n", errno);
                        break;
                    }
                    if (http_conn::m_user_count >= MAX_FD) {
                        show_errno(connfd, "Internal server busy");
                        break;
                    }
                    /* 初始化客户连接 */
                    users[connfd].init(connfd, client_address);
                }
                continue;
#endif
            } else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                /* 发生异常，直接关闭客户连接 */
                users[sockfd].close_conn();
            } else if ((sockfd == pipefd[0]) && (events[i].events & EPOLLIN)) {
                /* 根据读的结果，决定是将任务添加到线程池，还是关闭连接 */
                if (users[sockfd].read()) {
                    pool->append(users + sockfd);
                } else {
                    users[sockfd].close_conn();
                }
            } else if (events[i].events & EPOLLOUT) {
                /* 根据写的结果，决定是否关闭连接*/
                if (users[sockfd].write()) {
                    users[sockfd].close_conn();
                }
            }
        }
    }

    close(epollfd);
    close(listenfd);
    delete[] users;
    delete pool;

    return 0;
}