//
// Created by polarnight on 24-3-17, 下午10:35.
//

#ifndef WEBSERVER_LOCKER_H
#define WEBSERVER_LOCKER_H

/* 线程同步机制包装类 */

#include <semaphore.h>
#include <exception>
#include <pthread.h>

/* 封装信号量的类 */
class sem {
public:
    /* 创建并初始化信号量 */
    sem() {
        if (sem_init(&m_sem, 0, 0) != 0) {
            throw std::exception();
        }
    };

    /* 销毁信号量 */
    ~sem() {
        sem_destroy(&m_sem);
    };

    /* 等待信号量 */
    bool wait() {
        return sem_wait(&m_sem) == 0;
    }

    /* 增加信号量 */
    bool post() {
        return sem_post(&m_sem) == 0;
    }

private:
    sem_t m_sem;
};

/* 封装互斥量的类 */
class mutex {
public:
    /* 创建并初始化互斥量 */
    mutex() {
        if (pthread_mutex_init(&m_mutex, nullptr) != 0) {
            throw std::exception();
        }
    }

    /* 销毁互斥量 */
    ~mutex() {
        pthread_mutex_destroy(&m_mutex);
    }

    /* 获取互斥量 */
    bool locke() {
        return pthread_mutex_lock(&m_mutex) == 0;
    }

    /* 释放互斥量 */
    bool unlock() {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }

private:
    pthread_mutex_t m_mutex;
};

/* 封装条件变量的类 */
class cond {
public:
    /* 创建并初始化条件变量 */
    cond() {
        if (pthread_cond_init(&m_cond, nullptr) != 0) {
            throw std::exception();
        }
        if (pthread_mutex_init(&m_mutex, nullptr) != 0) {
            pthread_cond_destroy(&m_cond);
            throw std::exception();
        }
    }

    /* 销毁条件变量 */
    ~cond() {
        pthread_mutex_destroy(&m_mutex);
        pthread_cond_destroy(&m_cond);
    }

    /* 等待条件变量 */
    bool wait() {
        int ret = 0;
        pthread_mutex_lock(&m_mutex);
        ret = pthread_cond_wait(&m_cond, &m_mutex);
        pthread_mutex_unlock(&m_mutex);
        return ret == 0;
    }

    /* 唤醒等待条件变量的线程 */
    bool signal() {
        return pthread_cond_signal(&m_cond) == 0;
    }

private:
    pthread_cond_t m_cond;
    pthread_mutex_t m_mutex;
};

#endif //WEBSERVER_LOCKER_H
