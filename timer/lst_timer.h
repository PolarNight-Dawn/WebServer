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

#include "../log/log.h"

using std::exception;

#define BUFFER_SIZE 64

class heap_timer;

/* 连接资源 */
struct client_data {
    sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];
    heap_timer *timer;
};

/* 定时器类 */
class heap_timer {
public:
    heap_timer(int delay);

public:
    time_t expire;

    void (*cb_func)(client_data *);

    client_data *user_data;
};

/* 时间堆类 */
class time_heap {
public:
    /* 初始化一个大小为 cap 的空堆 */
    time_heap(int cap);

    /* 用已有数组来初始化堆 */
    time_heap(heap_timer **init_array, int size, int capacity);

    /* 销毁时间堆 */
    ~time_heap();

public:
    /* 添加目标定时器 */
    void add_timer(heap_timer *timer);

    /* 删除目标定时器 */
    void del_timer(heap_timer *timer);

    /* 获取堆顶部的定时器 */
    heap_timer *top() const;

    /* 删除堆顶部的定时器 */
    void pop_timer();

    /* 心搏函数 */
    void tick();

    bool empty() const;

private:
    /* 下虑函数 */
    void percolate_down(int hole);

    /* 扩容函数 */
    void resize();

private:
    heap_timer **array;
    int capacity;
    int cur_size;
};


#endif //WEBSERVER_LST_TIMER_H