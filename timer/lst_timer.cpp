//
// Created by polarnight on 24-3-20, 上午1:32.
//

#include "lst_timer.h"

heap_timer::heap_timer(int delay) {
    expire = time(nullptr) + delay;
}

/* 初始化一个大小为 cap 的空堆 */
time_heap::time_heap(int cap) : capacity(cap), cur_size(0) {
    array = new heap_timer *[capacity];
    if (!array) throw std::exception();

    for (int i = 0; i < capacity; ++i)
        array[i] = nullptr;
}

/* 用已有数组来初始化堆 */
time_heap::time_heap(heap_timer **init_array, int size, int capacity) : capacity(capacity), cur_size(size) {
    if (capacity < cur_size) throw std::exception();

    array = new heap_timer *[capacity];
    if (!array) throw std::exception();

    for (int i = 0; i < capacity; ++i)
        array[i] = nullptr;
    if (size > 0) {
        for (int i = 0; i < size; ++i)
            array[i] = init_array[i];
        for (int i = (cur_size - 1) / 2; i >= 0; --i)
            percolate_down(i);
    }
}

/* 销毁时间堆 */
time_heap::~time_heap() {
    for (int i = 0; i < cur_size; ++i)
        delete array[i];
    delete[] array;
}

/* 添加目标定时器 */
void time_heap::add_timer(heap_timer *timer) {
    if (!timer) return;
    if (capacity < cur_size) resize();

    int hole = ++cur_size;
    int parent = 0;
    for (; hole > 0; hole = parent) {
        parent = (hole - 1) / 2;
        if (array[parent]->expire <= array[hole]->expire) break;
        array[hole] = array[parent];
    }
    array[hole] = timer;
}

/* 删除目标定时器 */
void time_heap::del_timer(heap_timer *timer) {
    if (!timer) return;
    timer->cb_func = nullptr;
}

/* 获取堆顶部的定时器 */
heap_timer *time_heap::top() const {
    if (empty()) return nullptr;
    return array[0];
}

/* 删除堆顶部的定时器 */
void time_heap::pop_timer() {
    if (empty()) return;
    if (array[0]) {
        delete array[0];
        array[0] = array[--cur_size];
        percolate_down(0);
    }
}

/* 心搏函数 */
void time_heap::tick() {
    heap_timer *tmp = array[0];
    time_t cur = time(nullptr);
    while (!empty()) {
        if (!tmp) break;
        if (tmp->expire > cur) break;
        if (tmp->cb_func) tmp->cb_func(tmp->user_data);
        pop_timer();
        tmp = array[0];
    }
}

bool time_heap::empty() const { return cur_size == 0; }

/* 下虑函数 */
void time_heap::percolate_down(int hole) {
    heap_timer *tmp = array[hole];
    int child = 0;
    for (; hole * 2 + 1 < cur_size; hole = child) {
        child = hole * 2 + 1;
        if ((child + 1 < cur_size) && (array[child + 1]->expire < array[child]->expire)) ++child;
        if (array[child]->expire < tmp->expire) array[hole] = array[child];
        else break;
    }
    array[hole] = tmp;
}

/* 扩容函数 */
void time_heap::resize() {
    heap_timer **tmp = new heap_timer *[capacity * 2];
    if (!tmp) throw std::exception();

    for (int i = 0; i < 2 * capacity; i++) tmp[i] = nullptr;
    capacity = capacity * 2;
    for (int i = 0; i < cur_size; i++) tmp[i] = array[i];
    delete[] array;
    array = tmp;
}


