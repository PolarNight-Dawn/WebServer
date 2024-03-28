//
// Created by polarnight on 24-3-27, 上午11:23.
//

#ifndef WEBSERVER_BLOCK_QUEUE_H
#define WEBSERVER_BLOCK_QUEUE_H

#include <pthread.h>
#include <sys/time.h>

using namespace std;

template<typename T>
class block_queue {
public:
    /* 初始化阻塞队列 */
    block_queue(int max_size = 1000) : m_max_size(max_size), m_size(0), m_front(-1), m_back(-1) {
        m_array = new T[max_size];
        m_mutex = new pthread_mutex_t;
        m_cond = new pthread_cond_t;
        pthread_mutex_init(m_mutex, nullptr);
        pthread_cond_init(m_cond, nullptr);
    }

    /* 清除阻塞队列 */
    void clear() {
        pthread_mutex_lock(m_mutex);
        m_size = 0;
        m_front = -1;
        m_back = -1;
        pthread_mutex_unlock(m_mutex);
    }

    /* 销毁阻塞队列 */
    ~block_queue() {
        pthread_mutex_lock(m_mutex);
        if (m_array != nullptr) delete m_array;
        pthread_mutex_unlock(m_mutex);
    }

    /* 判断队列是否满了 */
    bool full() const {
        pthread_mutex_lock(m_mutex);
        if (m_size >= m_max_size) {
            pthread_mutex_unlock(m_mutex);
            return true;
        }
        pthread_mutex_unlock(m_mutex);
        return false;
    }

    /* 判断队列是否为空 */
    bool empty() const {
        pthread_mutex_lock(m_mutex);
        if (m_size == 0) {
            pthread_mutex_unlock(m_mutex);
            return true;
        }
        pthread_mutex_unlock(m_mutex);
        return false;
    }

    /* 返回队首元素 */
    bool front(T &value) const {
        pthread_mutex_lock(m_mutex);
        if (m_size == 0) {
            pthread_mutex_unlock(m_mutex);
            return false;
        }
        value = m_array[m_front];
        pthread_mutex_unlock(m_mutex);
        return true;
    }

    /* 返回队尾元素 */
    bool back(T &value) const {
        pthread_mutex_lock(m_mutex);
        if (m_size == 0) {
            pthread_mutex_unlock(m_mutex);
            return false;
        }
        value = m_array[m_back];
        pthread_mutex_unlock(m_mutex);
        return true;
    }

    int size() const {
        int tmp = 0;
        pthread_mutex_lock(m_mutex);
        tmp = m_size;
        pthread_mutex_unlock(m_mutex);
        return tmp;
    }

    int max_size() const {
        int tmp = 0;
        pthread_mutex_lock(m_mutex);
        tmp = m_max_size;
        pthread_mutex_unlock(m_mutex);
        return tmp;
    }

    /* 往队列里添加元素 */
    bool push(const T &item) {
        pthread_mutex_lock(m_mutex);
        if (m_size >= m_max_size) {
            pthread_cond_broadcast(m_cond);
            pthread_mutex_unlock(m_mutex);
            return false;
        }

        m_back = (m_back + 1) % m_max_size;
        m_array[m_back] = item;
        m_size++;
        pthread_cond_broadcast(m_cond);
        pthread_mutex_unlock(m_mutex);

        return true;
    }

    /* 从队列中删除元素 */
    bool pop(T &item) {
        pthread_mutex_lock(m_mutex);
        while (m_size <= 0) {
            if (pthread_cond_wait(m_cond, m_mutex) != 0 ) {
                pthread_mutex_unlock(m_mutex);
                return false;
            }
        }

        m_front = (m_front + 1) & m_max_size;
        item = m_array[m_front];
        m_size--;
        pthread_mutex_unlock(m_mutex);

        return true;
    }

    /* 超时处理 */
    bool pop(T &item, int m_timeout) {
        struct timesepc t = {0, 0};
        struct timeval now = {0, 0};
        gettimeofday(&now, nullptr);
        pthread_mutex_lock(m_mutex);
        if (m_size <= 0) {
            t.tv_sec = now.tv_sec + m_timeout / 1000;
            t.tv_nsec = (m_timeout % 1000) / 1000;
            if (pthread_cond_timedwait(m_cond, m_mutex, &t) != 0) {
                pthread_mutex_unlock(m_mutex);
                return false;
            }
        }

        if (m_size<= 0) {
            pthread_mutex_unlock(m_mutex);
            return false;
        }

        m_front = (m_front + 1) & m_max_size;
        item = m_array[m_front];
        m_size--;
        pthread_mutex_unlock(m_mutex);

        return true;
    }

private:
    int m_max_size;
    int m_size;
    int m_front;
    int m_back;
    T *m_array;
    pthread_mutex_t *m_mutex;
    pthread_cond_t *m_cond;
};

#endif //WEBSERVER_BLOCK_QUEUE_H
