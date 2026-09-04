#pragma once

#include <condition_variable>
#include <mutex>
#include <any>
#include <cstdint>

//错误码集合
//11xxx - 通用错误码
//11001 - 未知命令
//11002 - 配置无效
//11003 - 空指针异常
//11004 - 类型不匹配
//11005 - 资源创建失败
//11006 - 资源释放失败
//11007 - 线程创建失败
//11008 - 超时错误
//11009 - 状态无效
//11010 - 参数越界

//11101 - 初始化-句柄无效
//11102 - 初始化-获取DC失败
//11103 - 初始化-创建内存DC失败
//11104 - 初始化-创建位图失败
//11105 - 初始化-获取窗口矩形失败
//11106 - 初始化-尺寸无效

//11201 - 截屏-PrintWindow失败
//11202 - 截屏-GetDIBits失败
//11203 - 截屏-获取位图信息失败
//11204 - 截屏-输出Mat无效

//11301 - 暂停-状态无效
//11302 - 恢复-状态无效

//11401 - 卸载-状态无效
//11402 - 卸载-线程Join失败

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
enum class ConfigType : uint8_t {
    HWND = 1,
    FPS = 2,
    MAT = 3,
    STRING = 4,
    INT = 5
};

struct working_config {
    ConfigType type;
    std::any info;
};

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
    virtual int IWroking_cofig(working_config config) = 0;

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