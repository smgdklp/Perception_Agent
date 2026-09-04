#pragma once

#include "common.h"
#include <opencv2/opencv.hpp>
#include <thread>
#include <windows.h>
#include <chrono>
#include <atomic>
#include <any>

//!!!!!!!!!注意!!!!!!!!!!!!!!!
//类内配置没锁!!!!!!!!!!!!!!!
//记住要停下来再改配置
//(什么?我都写了暂停了还要加个锁增加维护复杂度吗>>>)
//!!!!!!!!!!!注意!!!!!!
//SetProcessDPIAware()要加
//该死的DPI害的我调试几天

//简单win截屏类
// 输入配置:HWND,fps(自动转化)
// 输出配置:cv::Mat

// ============================================================
// 错误码集合（WinCapture专用）
// ============================================================
// 11xxx - 通用错误码（见 common.h）
//
// 初始化错误（11100-11199）
// 11101 - 初始化-句柄无效
// 11102 - 初始化-获取DC失败
// 11103 - 初始化-创建内存DC失败
// 11104 - 初始化-创建位图失败
// 11105 - 初始化-获取窗口矩形失败
// 11106 - 初始化-尺寸无效
//
// 截屏错误（11200-11299）
// 11201 - 截屏-PrintWindow失败
// 11202 - 截屏-GetDIBits失败
// 11203 - 截屏-获取位图信息失败
// 11204 - 截屏-输出Mat无效
//
// 暂停/恢复错误（11300-11399）
// 11301 - 暂停-状态无效
// 11302 - 恢复-状态无效
//
// 卸载错误（11400-11499）
// 11401 - 卸载-状态无效
// 11402 - 卸载-线程Join失败
// ============================================================

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

    int _dpiX;
    int _dpiY;
    double _scaleX;
    double _scaleY;

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