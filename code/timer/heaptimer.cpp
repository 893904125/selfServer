//
// Created by szy on 2025/11/24.
//

#include "heaptimer.h"

#include <future>

/*
 * 需要判断id是否已经在heap中了
 */
void HeapTimer::add(int id, int timeout, const TimerOutCallBack& cb) {
    assert(id >= 0);
    size_t index;
    if (ref_.count(id) == 0) {
        //新节点
        index = heap_.size();
        ref_[id] = index;
        heap_.push_back({id, Clock::now() + MS(timeout), cb});
        //向上调整
        siftup_(index);
    }else {
        //已经在heap中了，修改时间，回调函数
        index = ref_[id];
        heap_[index].expires = Clock::now() + MS(timeout);
        heap_[index].cb = cb;
        if (!siftdown_(index, heap_.size())) {
            siftup_(index);
        }
    }
}

void HeapTimer::adjust(int id, int timeout) {
    assert(!heap_.empty() && ref_.count(id) > 0);
    size_t index = ref_[id];
    heap_[index].expires = Clock::now() + MS(timeout);
    if (!siftdown_(index, heap_.size())) {
        siftup_(index);
    }
}

void HeapTimer::doWork(int id) {
    assert(!heap_.empty() && ref_.count(id) > 0);
    size_t index = ref_[id];
    TimerNode node = heap_[index];
    node.cb();
    del_(index);
}

void HeapTimer::clear() {
    ref_.clear();
    heap_.clear();
}

/*
 * 取出超时的节点运行
 */
void HeapTimer::tick() {
    if (heap_.empty()) {
        return;
    }
    while (!heap_.empty()) {
        TimerNode node = heap_.front();
        if (std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0) {
            break;
        }
        node.cb();
        pop();
    }
}

void HeapTimer::pop() {
    assert(!heap_.empty());
    del_(0);
}

int HeapTimer::GetNextTick() {
    tick();
    size_t res = -1;
    if (!heap_.empty()) {
        res = std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count();
        if (res < 0){res = 0;}
    }
    return res;
}

/*
 * 删除把要删除的节点换到队尾，然后调整堆
 */
void HeapTimer::del_(size_t index) {
    assert(!heap_.empty() && index >= 0 && index < heap_.size());
    size_t i = index;
    size_t n = heap_.size() - 1;
    assert(i <= n);
    if (i < n) {
        SwapNode_(i, n);
        if (!siftdown_(i, n)) {
            siftup_(i);
        }
    }
    //删除队尾元素
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}

void HeapTimer::siftup_(size_t index) {
    assert(index >= 0 && index < heap_.size());
    size_t i = index;
    size_t j = (i - 1) / 2;
    while (j >= 0) {
        if (heap_[j] < heap_[i]) {
            break;
        }
        SwapNode_(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}

bool HeapTimer::siftdown_(size_t index, size_t n) {
    assert(index >= 0 && index < heap_.size());
    assert(n >= 0 && n <= heap_.size());
    size_t i = index;
    size_t j = i * 2 + 1;
    //先找出左右子节点中较小值
    while (j < n) {
        if (j + 1 < n && heap_[j+1]< heap_[j]) ++j;
        if (heap_[i] < heap_[j]) {break;}
        SwapNode_(i, j);
        i = j;
        j = i * 2 + 1;
    }
    //返回换没换
    return i > index;
}

void HeapTimer::SwapNode_(size_t i, size_t j) {
    assert(i >= 0 && i < heap_.size());
    assert(j >= 0 && j < heap_.size());
    std::swap(heap_[i], heap_[j]);
    ref_[heap_[i].id] = i;
    ref_[heap_[j].id] = j;
}