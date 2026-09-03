#pragma once

#include <vector>
#include <mutex>
#include <opencv2/opencv.hpp>

//封装的单帧缓存类模板
//为匹配对象池要做到每个内部缓存一样大小预留
struct Frame {
    uint64_t timestep;      //缓存时间戳当作特异标识
    int w_state;            //写状态
    int r_count;            //读计数
    cv::Mat frame;          //1020, 1980, 四通道

    Frame() : timestep(0), w_state(0), r_count(0) {}
};

//特异设计的帧缓存队列池
//预留连续内存多个帧对象，帧对象要内部预览尺寸相同

//GetFrame可以获取帧指针
//SetFrame获得指针，覆写一个空的对象，改变计数

//state状态表示当前帧写状态：0，空闲；1，在写；2，可读
//配合r_count计数，计数为0且w_state==0||2判断为空闲可以覆写

//呃呃呃，动态拓展还没写，一定要严格释放计数和严管数量！！！！不然会爆队列！！！！！
//要用一帧统一分发，通过索引分发可能会出现引用问题

class FramePool {
private:
    //预留帧尺寸
    int _h;
    int _w;
    int _t;

    //池子配置
    int _framesnum;         //池子缓存预留对象

    //池子对象
    int _framecurser;       //遍历池子的索引
    std::vector<Frame> _frames_pool;    //帧池
    std::mutex _cursorMutex;            //锁_framecurser

    //中间层转状态机
    //帧状态
    enum class State { Empty, Writing, Reading, Readable };

    State GetState(Frame* frame);
    //命令
    enum class Command { Write, Read };
    //映射: Write -> {Empty, Readable}, Read -> {Readable, Reading}
    bool StateMatch(State state, Command cmd);
    Frame* MovePtr(Command cmd);

public:
    FramePool(int h, int w, int t, int framesnum = 20);

    //做好计数做好计数
    //传入一个指针，赋值为一个可读Frame指针，只读
    int GetFrame(Frame*& curframe);

    //传入一个指针，赋值为一个空闲Frame指针，只写
    int SetFrame(Frame*& curframe);

    //完成写入，标记为可读
    void FinishWrite(Frame* frame);

    //释放读计数
    void ReturnFrame(Frame* frame);
};