//
// Created by polarnight on 24-3-20, 上午1:32.
//

#ifndef WEBSERVER_LST_TIMER_H
#define WEBSERVER_LST_TIMER_H


#include <unistd.h>
#include <csignal>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cassert>
#include <sys/stat.h>
#include <cstring>
#include <pthread.h>
#include <cstdio>
#include <cstdlib>
#include <sys/mman.h>
#include <cstdarg>
#include <cerrno>
#include <sys/wait.h>
#include <sys/uio.h>
#include <ctime>

#include "log.h"

class util_timer;

/* 连接资源 */
struct client_data {
    /* 客户端 socket 地址 */
    sockaddr_in address;
    /* socket 文件描述符 */
    int sockfd;
    /* 定时器 */
    util_timer *timer;
};

/* 定时器类 */
class util_timer {
public:
    util_timer() : prev(nullptr), next(nullptr) {}

public:
    /* 超时时间 */
    time_t expire;

    /* 回调函数 */
    void (*cb_func)(client_data *);

    /* 连接资源 */
    client_data *user_data;
    /* 前向定时器 */
    util_timer *prev;
    /* 后继定时器 */
    util_timer *next;
};

/* 定时器容器类 */
class sort_timer_lst {
public:
    sort_timer_lst() : head(nullptr), tail(nullptr) {}

    ~sort_timer_lst();

    /* 添加定时器 */
    void add_timer(util_timer *timer);

    /* 调整定时器 */
    void adjust_timer(util_timer *timer);

    /* 删除定时器 */
    void del_timer(util_timer *timer);

    /* 定时任务处理函数 */
    void tick();

private:
    /* 调整链表内部节点 */
    void add_timer(util_timer *timer, util_timer *lst_head);

    util_timer *head;
    util_timer *tail;
};

class Utils {
public:
    Utils() {}

    ~Utils() {}

    void init(int timeslot);

    //对文件描述符设置非阻塞
    int setnonblocking(int fd);

    //将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

    //信号处理函数
    static void sig_handler(int sig);

    //设置信号函数
    void addsig(int sig, void(handler)(int), bool restart = true);

    //定时处理任务，重新定时以不断触发SIGALRM信号
    void timer_handler();

    void show_error(int connfd, const char *info);

public:
    static int *u_pipefd;
    sort_timer_lst m_timer_lst;
    static int u_epollfd;
    int m_TIMESLOT;
};

#endif //WEBSERVER_LST_TIMER_H
