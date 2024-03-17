//
// Created by polarnight on 24-3-18, 上午12:22.
//

/* 任务类函数实现 */

#include "http_conn.h"

/* 注册事件到 epoll 指定的内核事件表 */
int addfd(int epollfd, int fd, bool one_shot) {
    return 0;
}

/* 注销 epoll 内核事件表上的事件 */
int removefd(int epollfd, int fd) {
    return 0;
}

/* 修改 epoll 内核事件表上的事件 */
int movfd(int epollfd, int fd) {
    return 0;
}

http_conn::http_conn() {

}

http_conn::~http_conn() {

}

/* 处理客户请求 */
void http_conn::process() {

}

/* 初始化客户连接 */
void http_conn::init(int connd, struct sockaddr_in address) {

}

/* 关闭客户连接 */
void http_conn::close_conn() {

}

/* 读取客户数据 */
bool http_conn::read() {

}

/* 写入客户数据 */
bool http_conn::write() {

}