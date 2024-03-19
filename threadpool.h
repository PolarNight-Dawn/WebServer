//
// Created by polarnight on 24-3-17, 下午11:04.
//

#ifndef WEBSERVER_THREADPOOL_H
#define WEBSERVER_THREADPOOL_H

/* 半同步/半反应堆线程池类 */

#include <list>

#include "locker.h"

template<typename T>
class threadpool {
public:
    /* 创建并初始化线程池 */
    threadpool(int thread_number = 8, int max_request = 1000);

    /* 销毁线程池 */
    ~threadpool();

    /* 往请求队列中添加任务 */
    bool append(T *reuqest);

private:
    /* 工作线程运行的函数，不断从请求列表中取出任务并执行 */
    static void *worker(void *arg);

    void run();

private:
    /* 线程池中线程的数量 */
    int m_thread_number;
    /* 请求队列中最大允许的数量 */
    int m_max_request;
    /* 描述线程池的数组 */
    pthread_t *m_threads;
    /* 请求队列 */
    std::list<T *> m_workqueue;
    /* 保护请求队列的互斥锁 */
    locker m_queuelocker;
    /* 是否有任务是要处理 */
    sem m_queuestat;
    /* 是否结束线程 */
    bool m_stop;
};

/* 创建并初始化线程池 */
template<typename T>
threadpool<T>::threadpool(int thread_number, int max_request) : m_thread_number(thread_number),
                                                                m_max_request(max_request), m_threads(nullptr),
                                                                m_stop(false) {
    if (thread_number <= 0 || max_request <= 0)
        throw std::exception();

    /* 初始化线程池数组 */
    m_threads = new pthread_t[m_thread_number];
    if (!m_threads)
        throw std::exception();

    /* 创建子线程，并设置为脱离线程 */
    for (int i = 0; i < thread_number; i++) {
        /* this 指针将线程池对象的地址传递给 worker 函数， arg 参数接受 this 指针 */
        if (pthread_create(m_threads + i, nullptr, worker, this) != 0) {
            delete[] m_threads;
            throw std::exception();
        }

        if (pthread_detach(m_threads[i]) != 0) {
            delete[] m_threads;
            throw std::exception();
        }
    }
}

/* 销毁线程池 */
template<typename T>
threadpool<T>::~threadpool() {
    delete[] m_threads;
}

/* 往请求队列中添加任务 */
template<typename T>
bool threadpool<T>::append(T *reuqest) {
    m_queuelocker.lock();
    if (m_workqueue.size() > m_max_request) {
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(reuqest);
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}

/* 子线程运行的工作函数，不断从请求列表中取出任务并执行 */
template<typename T>
void *threadpool<T>::worker(void *arg) {
    /* 静态成员函数 worker 没法调用类的非静态成员函数 */
    threadpool *pool = (threadpool *) arg;
    pool->run();
    return pool;
}

template<typename T>
void threadpool<T>::run() {
    while (!m_stop) {
        m_queuestat.wait();
        m_queuelocker.lock();
        if (m_workqueue.empty()) {
            m_queuelocker.unlock();
            return;
        }
        T *request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if (!request)
            continue;

        request->process();
    }
}

#endif //WEBSERVER_THREADPOOL_H
