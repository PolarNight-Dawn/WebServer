//
// Created by polarnight on 24-3-18, 上午12:22.
//

#ifndef WEBSERVER_HTTP_CONN_H
#define WEBSERVER_HTTP_CONN_H

/* 任务类函数声明 */

#include <sys/epoll.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <cstdarg>

#include "../lock/locker.h"

class http_conn {
public:
    /* 文件路径的最大长度 */
    static const int FILENAME_LEN = 200;
    /* 读缓冲区的大小 */
    static const int READ_BUFFER_SIZE = 2048;
    /* 写缓冲区的大小 */
    static const int WRITE_BUFFER_SIZE = 1024;
    /* HTTP 请求方法，现在仅支持 GET */
    enum METHOD {
        GET = 0, HEAD, POST, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATCH
    };
    /* 解析客户请求时， 主状态机所处的状态 */
    enum CHECK_STATE {
        CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT
    };
    /* 服务器处理 HTTP 请求时可能的结果 */
    enum HTTP_CODE {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
    /* 从状态机（行）的读取状态 */
    enum LINE_STATUS {
        LINE_OK = 0, LINE_BAD, LINE_OPEN
    };

public:
    http_conn() {}

    ~http_conn() {}

public:
    /* 处理客户请求 */
    void process();

    /* 初始化客户连接 */
    void init(int sockfd, const sockaddr_in &address);

    /* 关闭客户连接 */
    void close_conn(bool real_close = true);

    /* 非阻塞读操作 */
    bool read();

    /* 非阻塞写操作 */
    bool write();

public:
    /* 初始化连接 */
    void init();

    /* 解析 HTTP 请求 */
    HTTP_CODE process_read();

    /* 填充 HTTP 应答 */
    bool process_write(HTTP_CODE ret);

    /* 一组由 process_read() 调用来解析 HTTP 请求的函数 */
    LINE_STATUS parse_line();

    char *get_line() { return m_read_buf + m_start_line; }

    HTTP_CODE parse_request_line(char *text);

    HTTP_CODE parse_headers(char *text);

    HTTP_CODE parse_content(char *text);

    HTTP_CODE do_request();

    /* 一组由 process_write() 调用来回复 HTTP 响应的函数 */
    void unmap();

    bool add_response(const char *format, ...);

    bool add_status_line(int status, const char *title);

    bool add_header(int content_len);

    bool add_content_length(int content_len);

    bool add_content_type();

    bool add_linger();

    bool add_blank_line();

    bool add_content(const char *content);

public:
    /* 设置所有 socket 上的事件注册到一个 epoll 内核事件表 */
    static int m_epollfd;
    /* 统计用户总数 */
    static int m_user_count;

private:
    /* 读 HTTP 连接的 sockte 和 对方的 socket 地址 */
    int m_sockfd;
    sockaddr_in m_address;

    /* 读缓冲区 */
    char m_read_buf[READ_BUFFER_SIZE];
    /* 已经读入的数据的最后一个字节的下一位 */
    int m_read_idx;
    /* 写缓冲区 */
    char m_write_buf[WRITE_BUFFER_SIZE];
    /* 写缓冲区中待发送的字节数 */
    int m_write_idx;
    /* 当前正在解析的字符在读写缓冲区中的位置 */
    int m_check_idx;
    /* 当前正在解析的行的在读缓冲区中的位置 */
    int m_start_line;

    /* 主状态机当前所处的状态 */
    CHECK_STATE m_check_state;
    /* 请求方法 */
    METHOD m_method;
    /* 用户名和密码 */
    char *m_string;

    /* 客户请求的目标文件的文件名 */
    char *m_url;
    /* HTTP 版本号 */
    char *m_version;
    /* 客户主机名*/
    char *m_host;
    /* HTTP 请求的消息体的长度 */
    int m_content_length;
    /* HTTP 请求是否要求保持连接 */
    bool m_linger;

    /* 客户请求的目标文件的文件路径 */
    char m_real_file[FILENAME_LEN];
    /* 目标文件的状态 */
    struct stat m_file_stat;
    /* 目标文件被映射到内存的首地址*/
    char *m_file_address;

    /* 通过内存块分散写 */
    struct iovec m_iv[2];
    int m_iv_count;

    int m_pipefd[2];

};


#endif //WEBSERVER_HTTP_CONN_H
