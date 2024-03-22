//
// Created by polarnight on 24-3-21, 下午8:36.
//

#include "log.h"

Log::Log() {
    m_count = 0;
}

Log::~Log() {
    if (!m_fp)
        fclose(m_fp);
}

bool Log::init(const char *file_name, int log_buf_size, int max_lines) {
    m_log_buf_size = log_buf_size;
    m_buf = new char[m_log_buf_size];
    memset(m_buf, '\0', sizeof(m_buf));

    m_max_lines = max_lines;

    time_t t = time(nullptr);
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    const char *ptr = strrchr(file_name, '/');
    char log_full_name[256] = {0};

    if (ptr == nullptr) {
        snprintf(log_full_name, 255, "%d_%02d_%02d_%s", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                 file_name);
    } else {
        strcpy(log_name, ptr + 1);
        strncpy(dir_name, file_name, ptr - file_name + 1);

        snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1,
                 my_tm.tm_mday, log_name);
    }

    m_today = my_tm.tm_mday;
    m_fp = fopen(log_full_name, "a");
    if (m_fp == nullptr)
        return false;
    return true;
}

void Log::write_log(int level, const char *format, ...) {
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t t = now.tv_sec;
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    char str[16] = {0};

    switch (level) {
        case 0:
            strcpy(str, "[debug]:");
            break;
        case 1:
            strcpy(str, "[info]:");
            break;
        case 2:
            strcpy(str, "[warn]:");
            break;
        case 3:
            strcpy(str, "[error]:");
            break;
        default:
            strcpy(str, "[info]:");
            break;
    }

    m_mutex.lock();

    m_count++;

    if (m_today != my_tm.tm_mday || m_count % m_max_lines == 0) {
        char new_log[256] = {0};
        fflush(m_fp);
        fclose(m_fp);
        char tail[16] = {16};

        snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);

        if (m_today != my_tm.tm_mday) {
            snprintf(new_log, 255, "%s%s%s", dir_name, tail, log_name);
            m_today = my_tm.tm_mday;
            m_count = 0;
        } else {
            snprintf(new_log, 255, "%s%s%s.%lld", dir_name, tail, log_name, m_count / m_max_lines);
        }
        m_fp = fopen(new_log, "a");
    }

    m_mutex.unlock();

    va_list valst;
    va_start(valst, format);

    string log_str;
    m_mutex.lock();
    int n = snprintf(m_buf, 48, "%d_%02d_%02d %02d:%02d:%02d.%06ld %s", my_tm.tm_year + 1900, my_tm.tm_mon + 1,
                     my_tm.tm_mday, my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_sec, str);
    int m = vsnprintf(m_buf + n, m_log_buf_size - 1, format, valst);
    m_buf[n + m] = '\n';
    m_buf[n + m + 1] = '\0';

    log_str = m_buf;

    fputs(log_str.c_str(), m_fp);
    m_mutex.unlock();
    va_end(valst);
}

void Log::flush(void) {
    m_mutex.lock();
    fflush(m_fp);
    m_mutex.unlock();
}
