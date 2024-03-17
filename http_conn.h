//
// Created by polarnight on 24-3-18, 上午12:22.
//

#ifndef WEBSERVER_HTTP_CONN_H
#define WEBSERVER_HTTP_CONN_H

/* 任务类函数声明 */

#include <sys/epoll.h>
#include <netinet/in.h>

class http_conn {
public:
    http_conn();

    ~http_conn();

public:
    /* 处理客户请求 */
    void process();

    /* 初始化客户连接 */
    void init(int connd, struct sockaddr_in address);

    /* 关闭客户连接 */
    void close_conn();

    /* 读取客户数据 */
    bool read();

    /* 写入客户数据 */
    bool write();

public:
    /* 设置所有 socket 上的事件注册到一个 epoll 内核事件表 */
    static int m_epollfd;
    /* 统计用户总数 */
    static int m_user_count;

private:


};


#endif //WEBSERVER_HTTP_CONN_H
