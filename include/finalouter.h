//!!!!!!!!!注意!!!!!!!!!!!!!!!
//RECT作为状态成员函数!!!!!!!!!!
//修改的时候要暂停工作

//简单图像输出终端
//因为包含裁剪图像，所以最终统一输出为
//灰度图和一个偏置坐标

//统一图像最终输出对象
//输入:RECT，Mat原始图像
//输出:Mat灰度裁剪图

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

// ============================================================
// 错误码集合（FinalOutput专用）
// ============================================================
// 初始化错误（13100-13199）
// 13101 - 初始化-输入Mat无效
// 13102 - 初始化-输出Mat无效
// 13103 - 初始化-配置缺失
//
// 处理错误（13200-13299）
// 13201 - 处理-拷贝帧失败
// 13202 - 处理-裁剪失败
// 13203 - 处理-灰度转换失败
// 13204 - 处理-保存失败
// 13205 - 处理-裁剪区域出界
//
// 线程错误（13300-13399）
// 13301 - 线程-创建失败
// 13302 - 线程-Join失败
// ============================================================

class FinalOutput : public IWorker {
private:
    // ====== 配置类 ======
    std::condition_variable* _downer;
    std::mutex* _downer_lock;

    std::condition_variable* _uper;
    std::mutex* _uper_lock;

    // ====== 配置参数 ======
    RECT _offset;               // 裁剪区域，默认{0,0,0,0}

    // ====== 共享内存 ======
    cv::Mat* _input;            // 输入原始图像
    cv::Mat* _output;           // 输出灰度裁剪图

    // ====== 内部参数 ======
    bool _iswork;
    bool _islife;

    std::thread _workThread;

    // ====== 缓存 ======
    cv::Mat _cache;             // 原始帧缓存
    cv::Mat _cut_cache;         // 裁剪后缓存

    // ====== 内部工作函数 ======
    int SaveFrame();            // 拷贝输入到_cache
    int Cut();                  // 按_offset裁剪到_cut_cache
    int TurnL();                // 转为单通道灰度图
    int Save();                 // 保存到_output

    // ====== 主线循环 ======
    void Work();

    // ====== 生命周期函数 ======
    int OnInit() override;
    int OnPause() override;
    int OnResume() override;
    int OnUnload() override;

public:
    FinalOutput();
    ~FinalOutput() override;

    // 禁止拷贝
    FinalOutput(const FinalOutput&) = delete;
    FinalOutput& operator=(const FinalOutput&) = delete;

    // ====== 配置槽函数 ======
    int ILock_config(lock_config config) override;
    int ICache_config(cache_config config) override;
    int IWroking_cofig(int cmd, std::any config) override;

    // ====== 命令槽函数 ======
    int IWorking_cmd(int cmd, void* input, void* output) override;
};