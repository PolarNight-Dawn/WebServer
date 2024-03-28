//
// Created by polarnight on 24-3-21, 下午8:36.
//

#ifndef WEBSERVER_LOG_H
#define WEBSERVER_LOG_H

/* 同步日志类 */

#include <iostream>
#include <cstring>
#include <sys/time.h>
#include <stdarg.h>
#include <string>

#include "../lock/locker.h"
#include "./block_queue.h"

using namespace std;

class Log {
public:
    /* 公有方法获取实例 */
    static Log *get_instance() {
        static Log instance;
        return &instance;
    }

    static void *flush_log_thread(void *args) {
        Log::get_instance()->async_write_log();
    }

    /* 初始化日志文件 */
    bool init(const char *file_name, int log_buf_size = 8193, int max_lines = 500000, int max_queue_size = 0);

    /* 内容格式化 */
    void write_log(int level, const char *format, ...);

    /* 强制刷新缓冲区 */
    void flush(void);

private:
    Log();

    virtual ~Log();

    /* 异步写入 */
    void *async_write_log() {
        string single_log;
        while (m_log_queue->pop(single_log)) {
            m_mutex.lock();
            fputs(single_log.c_str(), m_fp);
            m_mutex.unlock();
        }
    }

private:
    /* 路径名 */
    char dir_name[128];
    /* 日志名 */
    char log_name[128];
    /* 日志缓冲区大小 */
    int m_log_buf_size;
    /* 日志最大行数 */
    int m_max_lines;
    /* 日志行数纪录 */
    long long m_count
    /* 日志创建日期记录 */;
    int m_today;
    /* 指向日志的文件指针 */
    FILE *m_fp;
    /* 输出内容 */
    char *m_buf;
    /* 同步类 */
    locker m_mutex;
    /* 异步阻塞队列 */
    block_queue<string> *m_log_queue;
    /* 是否异步标志位 */
    bool m_is_async;
};

/* 定义四个宏， 用于不同类型的日志输出 */
#define LOG_DEBUG(format, ...) Log::get_instance()->write_log(0, format, ##__VA_ARGS__);
#define LOG_INFO(format, ...) Log::get_instance()->write_log(1, format, ##__VA_ARGS__);
#define LOG_WARN(format, ...) Log::get_instance()->write_log(2, format, ##__VA_ARGS__);
#define LOG_ERROR(format, ...) Log::get_instance()->write_log(3, format, ##__VA_ARGS__);

#endif //WEBSERVER_LOG_H
