//一个简单通过连续帧来提取窗口运动区域的类
//采用转化为明度二值图，简单帧差，最后每行阈值提取边界
//因为界面问题，外界自己复验结果(((())))

#pragma once

#include "common.h"
#include <opencv2/opencv.hpp>
#include <thread>
#include <windows.h>
#include <chrono>
#include <atomic>
#include <any>
#include <condition_variable>
#include <mutex>



// 初始化错误（12100-12199）
// 12101 - 初始化-输入Mat无效
// 12102 - 初始化-输出RECT无效
// 12103 - 初始化-配置缺失
//
// 处理错误（12200-12299）
// 12201 - 处理-帧差失败
// 12202 - 处理-二值化失败
// 12203 - 处理-边界提取失败
//
// 线程错误（12300-12399）
// 12301 - 线程-创建失败
// 12302 - 线程-Join失败
// ============================================================

class MainPartFinder : public IWorker {
private:
    // ====== 配置类 ======
    std::condition_variable* _downer;
    std::mutex* _downer_lock;

    std::condition_variable* _uper;
    std::mutex* _uper_lock;

    int _light_threshold;       // 分割阈值，默认100
    int _num_threshold;         // 边界筛选阈值，默认50

    // ====== 共享内存 ======
    cv::Mat* _input;            // 和上级生产者的缓存队列
    RECT* _output;              // 输出结果

    // ====== 内部参数 ======
    bool _iswork;
    bool _islife;

    int _cur_cmd;
    std::thread _workThread;

    // ====== 自身响应锁 ======
    std::condition_variable* _thiscv;
    std::mutex* _thismtx;

    // ====== 缓存 ======
    cv::Mat _pre;               // 预留1920*1080*4
    cv::Mat _cur;
    cv::Mat _light;             // 帧差cache
    RECT _persize;

    // ====== 内部工作函数 ======
    int GetFrame();             // 拷贝
    int TurnL();                // 转化为二值图
    int Framecut();             // 帧差
    int Threshold();            // 硬二分
    int GetMain();              // 提取主区域边界

    // ====== 主线循环 ======
    void Work();

    // ====== 生命周期函数 ======
    int OnInit() override;
    int OnPause() override;
    int OnResume() override;
    int OnUnload() override;

public:
    MainPartFinder();
    ~MainPartFinder() override;

    // 禁止拷贝
    MainPartFinder(const MainPartFinder&) = delete;
    MainPartFinder& operator=(const MainPartFinder&) = delete;

    // ====== 配置槽函数 ======
    int ILock_config(lock_config config) override;
    int ICache_config(cache_config config) override;
    int IWroking_cofig(working_config config) override;

    // ====== 命令槽函数 ======
    int IWorking_cmd(int cmd, void* input, void* output) override;
};