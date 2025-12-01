//
// Created by szy on 2025/11/24.
//

#ifndef SELFSERVER_HEAPTIMER_H
#define SELFSERVER_HEAPTIMER_H
#include <functional>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <assert.h>



typedef std::function<void()> TimerOutCallBack;
typedef std::chrono::high_resolution_clock::time_point TimeStamp;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;

//用TimerNode 存储 id, 超时时间 和 超时事件
struct TimerNode {
    int id;
    TimeStamp expires;
    TimerOutCallBack cb;

    //重构一下比较运算符，方便比较节点的超时时间
    bool operator<(const TimerNode & node) const {
        return expires < node.expires;
    }
};

/*
 * 用小根堆存储节点，可以快速取出最小超时时间的事件
 * 小根堆使用vector实现， 添加一个ref,建立id和坐标的索引， 方便快速查找
 */

class HeapTimer {
public:
    /*告诉 vector 预留 64 个位置的内存，避免频繁扩容，但不要真正创建*/
    HeapTimer() {heap_.reserve(64);}
    ~HeapTimer() {clear();};

    void add(int id, int timeout,const TimerOutCallBack& cb);

    void adjust(int id, int timeout);

    void doWork(int id);

    void clear();

    void tick();

    void pop();

    int GetNextTick();
private:
    void del_(size_t index);

    void siftup_(size_t i);

    bool siftdown_(size_t i, size_t n);

    void SwapNode_(size_t i, size_t j);
private:
    std::vector<TimerNode> heap_;
    std::unordered_map<int, size_t> ref_;
};


#endif //SELFSERVER_HEAPTIMER_H