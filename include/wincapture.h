#pragma once

#include "common.h"
#include <opencv2/opencv.hpp>
#include <thread>
#include <chrono>
#include <atomic>
#include <any>
//!!!!!!!!!注意!!!!!!!!!!!!!!!
//类内配置没锁!!!!!!!!!!!!!!!
//记住要停下来再改配置
//(什么?我都写了暂停了还要加个锁增加维护复杂度吗>>>)

//简单win截屏类
// 输入配置:HWND,fps(自动转化)
// 输出配置:cv::Mat




//错误码集合（WinCapture专用）
//01xxxx - 通用错误码（见common.h）
//011001 - 初始化-句柄无效
//011002 - 初始化-获取DC失败
//011003 - 初始化-创建内存DC失败
//011004 - 初始化-创建位图失败
//011005 - 初始化-获取窗口矩形失败
//011006 - 初始化-尺寸无效
//011201 - 截屏-PrintWindow失败
//011202 - 截屏-GetDIBits失败
//011203 - 截屏-获取位图信息失败
//011204 - 截屏-输出Mat无效
//011301 - 暂停-状态无效
//011302 - 恢复-状态无效
//011401 - 卸载-状态无效
//011402 - 卸载-线程Join失败



class WinCapture : public IWorker {
private:
    //配置类
    std::condition_variable* _downer;
    std::mutex* _downer_lock;
    std::condition_variable* _uper;
    std::mutex* _uper_lock;

    //共享内存
    cv::Mat* _output;
    HWND _hwnd;
    int _intervalMs;            //帧间隔（毫秒）

    //内部参数
    HDC _winDC;                 //缓存屏幕dc配置
    HDC _curDC;                 //当前DC
    HBITMAP _initmap;           //储存初始画布用来销毁释放内存
    HBITMAP _curmap;            //当前画布

    std::atomic<bool> _iswork;
    std::atomic<bool> _islife;
    int _fps;

    //cache
    BITMAPINFO _info;
    std::chrono::steady_clock::time_point _lasttime;
    RECT _size;                 //缓存裁剪区域

    std::thread _workThread;

    //计时器
    void Timer();

    //捕获
    int Capture();

    //保存到Mat
    int Save();

    //主工作循环
    void Work();

    //主工作入口
    int MainWork();

    //生命周期函数
    int OnInit() override;
    int OnPause() override;
    int OnResume() override;
    int OnUnload() override;

public:
    WinCapture();
    ~WinCapture() override;

    //禁止拷贝
    WinCapture(const WinCapture&) = delete;
    WinCapture& operator=(const WinCapture&) = delete;

    //配置槽函数
    int ILock_config(lock_config config) override;
    int ICache_config(cache_config config) override;
    int IWroking_cofig(working_config config) override;

    //命令槽函数
    int IWorking_cmd(int cmd, void* input, void* output) override;
};