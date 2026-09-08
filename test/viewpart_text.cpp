#include "wincapture.h"
#include "mainpartfinder.h"
#include "finalouter.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <windows.h>
#include <opencv2/opencv.hpp>
#include <string>
#include <iomanip>
#include <sstream>
#include <vector>
#include <atomic>
#include <fstream>

// ============================================================
// Saver 类 - 简单图片保存器
// ============================================================
class Saver : public IWorker {
private:
    std::condition_variable* _uper;
    std::mutex* _uper_lock;
    cv::Mat* _input;
    std::string _savePath;
    bool _iswork;
    bool _islife;
    cv::Mat _cache;
    std::thread _workThread;

    std::string GetTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        struct tm tm_now;
        localtime_s(&tm_now, &time_t_now);
        std::ostringstream oss;
        oss << std::put_time(&tm_now, "%Y%m%d_%H%M%S")
            << "_" << std::setw(3) << std::setfill('0') << ms.count();
        return oss.str();
    }

    bool SaveMatAsBmp(const std::string& filename, const cv::Mat& mat) {
        if (mat.empty()) return false;
        
        cv::Mat saveMat;
        if (mat.channels() == 1) {
            cv::cvtColor(mat, saveMat, cv::COLOR_GRAY2BGRA);
        } else {
            saveMat = mat.clone();
        }

        int width = saveMat.cols;
        int height = saveMat.rows;
        int stride = width * 4;

        BITMAPFILEHEADER fh = {0};
        fh.bfType = 0x4D42;
        fh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + stride * height;
        fh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

        BITMAPINFOHEADER ih = {0};
        ih.biSize = sizeof(BITMAPINFOHEADER);
        ih.biWidth = width;
        ih.biHeight = -height;
        ih.biPlanes = 1;
        ih.biBitCount = 32;
        ih.biCompression = BI_RGB;
        ih.biSizeImage = 0;

        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;

        file.write(reinterpret_cast<const char*>(&fh), sizeof(fh));
        file.write(reinterpret_cast<const char*>(&ih), sizeof(ih));
        file.write(reinterpret_cast<const char*>(saveMat.data), stride * height);
        file.close();
        return true;
    }

    int Copy() {
        if (!_input || _input->empty()) return -1;
        _cache = _input->clone();
        return 0;
    }

    int SaveFrame() {
        if (_cache.empty()) return -1;
        std::string filename = _savePath + "\\" + GetTimestamp() + ".bmp";
        if (SaveMatAsBmp(filename, _cache)) {
            std::cout << "[Saver] ✅ 保存: " << filename << std::endl;
            return 0;
        }
        return -1;
    }

    void Work() {
        while (_islife) {
            if (_iswork) {
                if (_uper && _uper_lock) {
                    std::unique_lock<std::mutex> lock(*_uper_lock);
                    _uper->wait(lock, [this]() { return !_islife; });
                }
                if (!_islife) break;

                Copy();
                SaveFrame();
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }

    int OnInit() override {
        if (!_input) return -1;
        if (!_uper || !_uper_lock) return -1;
        _iswork = true;
        _islife = true;
        try {
            _workThread = std::thread(&Saver::Work, this);
        } catch (...) {
            return -1;
        }
        return 0;
    }

    int OnPause() override { _iswork = false; return 0; }
    int OnResume() override { _iswork = true; return 0; }
    int OnUnload() override {
        _islife = false;
        _iswork = false;
        if (_uper && _uper_lock) {
            _uper->notify_all();
        }
        if (_workThread.joinable()) {
            _workThread.join();
        }
        _cache.release();
        return 0;
    }

public:
    Saver() : _uper(nullptr), _uper_lock(nullptr), _input(nullptr), _iswork(false), _islife(true) {}
    ~Saver() { if (_islife) OnUnload(); }

    int ILock_config(lock_config config) override {
        if (config.uper) _uper = config.uper;
        if (config.uper_lock) _uper_lock = config.uper_lock;
        return 0;
    }
    int ICache_config(cache_config config) override {
        if (config.input) _input = reinterpret_cast<cv::Mat*>(config.input);
        if (config.output) {
            std::string* p = reinterpret_cast<std::string*>(config.output);
            if (p) _savePath = *p;
        }
        return 0;
    }
    int IWroking_cofig(int cmd, std::any config) override { return 0; }
    int IWorking_cmd(int cmd, void* input, void* output) override { return 0; }
    int ILiveing_cmd(int cmd) override {
        switch (cmd) {
        case 0: return OnInit();
        case 1: return OnPause();
        case 2: return OnResume();
        case 3: return OnUnload();
        default: return -1;
        }
    }
};

// ============================================================
// 工具函数
// ============================================================
std::string GetTimestamp2() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    struct tm tm_now;
    localtime_s(&tm_now, &time_t_now);
    std::ostringstream oss;
    oss << std::put_time(&tm_now, "%Y%m%d_%H%M%S") << "_" << std::setw(3) << std::setfill('0') << ms.count();
    return oss.str();
}

// ============================================================
// Main
// ============================================================
int main() {
    SetProcessDPIAware();
    SetConsoleOutputCP(CP_UTF8);

    std::cout << "========================================" << std::endl;
    std::cout << "      完整视觉部分测试程序" << std::endl;
    std::cout << "========================================" << std::endl;

    // ---- 创建共享Mat ----
    cv::Mat captureMat(1080, 1920, CV_8UC4);
    cv::Mat outputMat(1080, 1920, CV_8UC4);
    RECT offsetRect = {0, 0, 0, 0};

    // ---- 创建锁 ----
    std::mutex mtx1, mtx2, mtx3;
    std::condition_variable cv1, cv2, cv3;

    // ---- 创建对象 ----
    WinCapture* wincapture = new WinCapture();
    MainPartFinder* mainfinder = new MainPartFinder();
    FinalOutput* finaloutput = new FinalOutput();
    Saver* saver1 = new Saver();
    Saver* saver2 = new Saver();

    std::cout << "[Main] 所有对象创建成功" << std::endl;

    // ---- 配置缓存链路 ----
    cache_config cc1;
    cc1.input = nullptr;
    cc1.output = &captureMat;
    wincapture->ICache_config(cc1);

    cache_config cc2;
    cc2.input = &captureMat;
    cc2.output = &offsetRect;
    mainfinder->ICache_config(cc2);

    cache_config cc3;
    cc3.input = &captureMat;
    cc3.output = &outputMat;
    finaloutput->ICache_config(cc3);

    cache_config cc4;
    cc4.input = &captureMat;
    cc4.output = nullptr;
    saver1->ICache_config(cc4);

    cache_config cc5;
    cc5.input = &outputMat;
    cc5.output = nullptr;
    saver2->ICache_config(cc5);

    // ---- 配置锁链路 ----
    lock_config lc1;
    lc1.downer = &cv1;
    lc1.downer_lock = &mtx1;
    lc1.uper = nullptr;
    lc1.uper_lock = nullptr;
    wincapture->ILock_config(lc1);

    lock_config lc2;
    lc2.downer = nullptr;
    lc2.downer_lock = nullptr;
    lc2.uper = &cv1;
    lc2.uper_lock = &mtx1;
    mainfinder->ILock_config(lc2);

    lock_config lc3;
    lc3.downer = &cv2;
    lc3.downer_lock = &mtx2;
    lc3.uper = &cv1;
    lc3.uper_lock = &mtx1;
    finaloutput->ILock_config(lc3);

    lock_config lc4;
    lc4.downer = nullptr;
    lc4.downer_lock = nullptr;
    lc4.uper = &cv2;
    lc4.uper_lock = &mtx2;
    saver2->ILock_config(lc4);

    lock_config lc5;
    lc5.downer = nullptr;
    lc5.downer_lock = nullptr;
    lc5.uper = &cv3;
    lc5.uper_lock = &mtx3;
    saver1->ILock_config(lc5);

    // ---- 配置MainPartFinder参数 ----
    mainfinder->IWroking_cofig(1, 100);
    mainfinder->IWroking_cofig(2, 50);
    mainfinder->IWroking_cofig(3, 60);

    // ---- 设置Saver保存路径 ----
    std::string savePath = "D:\\CodingPrograms\\Perception_Agent\\resource\\finalTest";
    cache_config saverPath;
    saverPath.input = nullptr;
    saverPath.output = &savePath;
    saver1->ICache_config(saverPath);
    saver2->ICache_config(saverPath);

    // ---- 全部初始化且暂停 ----
    std::cout << "[Main] 初始化所有组件(暂停状态)..." << std::endl;
    wincapture->ILiveing_cmd(0);
    mainfinder->ILiveing_cmd(0);
    finaloutput->ILiveing_cmd(0);
    saver1->ILiveing_cmd(0);
    saver2->ILiveing_cmd(0);

    wincapture->ILiveing_cmd(1);
    mainfinder->ILiveing_cmd(1);
    finaloutput->ILiveing_cmd(1);
    saver1->ILiveing_cmd(1);
    saver2->ILiveing_cmd(1);

    std::cout << "[Main] 所有组件已初始化并暂停" << std::endl;
    std::cout << "[Main] 按 Enter 继续..." << std::endl;
    getchar();

    // ---- 等待3秒获取窗口 ----
    std::cout << "[Main] 等待3秒，请切换到目标窗口..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));

    HWND hwnd = GetForegroundWindow();
    if (hwnd) {
        char title[256];
        GetWindowTextA(hwnd, title, sizeof(title));
        std::cout << "[Main] ✅ 获取到窗口: " << title << std::endl;
    } else {
        std::cout << "[Main] ❌ 获取窗口失败" << std::endl;
    }

    // ---- 注入HWND ----
    wincapture->IWroking_cofig(1, hwnd);
    wincapture->IWroking_cofig(2, 30);
    std::cout << "[Main] ✅ 注入HWND和FPS完成" << std::endl;

    // ---- 启动除Saver外的所有组件 ----
    std::cout << "[Main] 启动 WinCapture, MainPartFinder, FinalOutput..." << std::endl;
    wincapture->ILiveing_cmd(2);
    mainfinder->ILiveing_cmd(2);
    finaloutput->ILiveing_cmd(2);

    // ============================================================
    // 测试一：保存每层结果
    // ============================================================
    std::cout << "\n========================================" << std::endl;
    std::cout << "[测试一] 保存每层结果" << std::endl;
    std::cout << "========================================" << std::endl;

    saver1->ILiveing_cmd(2);
    saver2->ILiveing_cmd(2);

    std::cout << "[Main] 等待3秒保存图片..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));

    saver1->ILiveing_cmd(1);
    saver2->ILiveing_cmd(1);
    std::cout << "[测试一] 完成" << std::endl;

    // ============================================================
    // 测试二：主成分提取
    // ============================================================
    std::cout << "\n========================================" << std::endl;
    std::cout << "[测试二] 主成分提取 (10次)" << std::endl;
    std::cout << "========================================" << std::endl;

    std::vector<RECT> results;
    for (int i = 0; i < 10; ++i) {
        RECT rect = {0, 0, 0, 0};
        mainfinder->IWorking_cmd(1, nullptr, &rect);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        results.push_back(rect);
        std::cout << "[Test2] 第" << (i + 1) << "次: "
                  << "left=" << rect.left << ", top=" << rect.top
                  << ", right=" << rect.right << ", bottom=" << rect.bottom
                  << " (宽=" << (rect.right - rect.left)
                  << ", 高=" << (rect.bottom - rect.top) << ")"
                  << std::endl;
    }

    // ============================================================
    // 测试三：联合测试
    // ============================================================
    std::cout << "\n========================================" << std::endl;
    std::cout << "[测试三] 联合测试" << std::endl;
    std::cout << "========================================" << std::endl;

    RECT finRect = {0, 0, 0, 0};
    int maxArea = 0;
    for (const auto& r : results) {
        int area = (r.right - r.left) * (r.bottom - r.top);
        if (area > maxArea) {
            maxArea = area;
            finRect = r;
        }
    }
    std::cout << "[Test3] 选择最大RECT: left=" << finRect.left
              << ", top=" << finRect.top
              << ", right=" << finRect.right
              << ", bottom=" << finRect.bottom
              << " (面积=" << maxArea << ")" << std::endl;

    finaloutput->IWroking_cofig(1, finRect);
    std::cout << "[Test3] 注入RECT到FinalOutput" << std::endl;

    saver1->ILiveing_cmd(2);
    saver2->ILiveing_cmd(2);
    std::cout << "[Main] 等待3秒保存裁剪结果..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    saver1->ILiveing_cmd(1);
    saver2->ILiveing_cmd(1);

    RECT smallRect = {10, 10, 10, 10};
    finaloutput->IWroking_cofig(1, smallRect);
    std::cout << "[Test3] 注入小RECT {10,10,10,10} 到FinalOutput" << std::endl;

    saver1->ILiveing_cmd(2);
    saver2->ILiveing_cmd(2);
    std::cout << "[Main] 等待3秒保存裁剪结果..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    saver1->ILiveing_cmd(1);
    saver2->ILiveing_cmd(1);

    std::cout << "[测试三] 完成" << std::endl;

    // ---- 释放所有内存 ----
    std::cout << "\n[Main] 释放所有内存..." << std::endl;

    wincapture->ILiveing_cmd(3);
    mainfinder->ILiveing_cmd(3);
    finaloutput->ILiveing_cmd(3);
    saver1->ILiveing_cmd(3);
    saver2->ILiveing_cmd(3);

    delete wincapture;
    delete mainfinder;
    delete finaloutput;
    delete saver1;
    delete saver2;

    captureMat.release();
    outputMat.release();

    std::cout << "[Main] ✅ 所有资源已释放" << std::endl;
    std::cout << "\n按 Enter 退出..." << std::endl;
    getchar();

    return 0;
}