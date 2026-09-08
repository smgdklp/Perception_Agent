#pragma once

#include <condition_variable>
#include <mutex>
#include <any>
#include <cstdint>

//


//基本生产组件类配置模板
//锁配置，传递锁索引
struct lock_config {
    //作为生产者的向下分发广播
    std::condition_variable* downer;
    //作为生产者的持有锁
    std::mutex* downer_lock;

    //作为消费者的广播
    std::condition_variable* uper;
    //作为消费者的锁
    std::mutex* uper_lock;
};

//共享内存配置，传递共享内存
struct cache_config {
    void* input;
    void* output;
};

//配置传输协议




//基本生产组件的模板类
//包含三个配置注入函数接口
//一个基础生命周期操控函数接口，只接收int的指令
//一个泛用函数接口函数，传入int指令和void* input ，void* output
class IWorker {
protected:
    IWorker() = default;

    //！！！配置槽，对于异性类自己管理好配置解析

    //！！！主工作函数，等待广播和分发广播

    //！！！统一接口的生命管理函数
    virtual int OnInit() = 0;
    virtual int OnPause() = 0;
    virtual int OnResume() = 0;
    virtual int OnUnload() = 0;

public:
    virtual ~IWorker() = default;

    //！！！配置槽函数
    virtual int ILock_config(lock_config config) = 0;
    virtual int ICache_config(cache_config config) = 0;
    virtual int IWroking_cofig(int cmd, std::any config) = 0;

    //命令槽函数
    virtual int ILiveing_cmd(int cmd) {
        switch (cmd) {
        case 0: return OnInit();      // 初始化
        case 1: return OnPause();     // 暂停
        case 2: return OnResume();    // 恢复
        case 3: return OnUnload();    // 卸载
        default: return 11001;        // 未知命令
        }
    }

    virtual int IWorking_cmd(int cmd, void* input, void* output) = 0;
};