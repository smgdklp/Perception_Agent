#include "wincapture.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <fstream>
#include <windows.h>
#include <iomanip>
#include <vector>

bool SaveMatAsBmp(const std::string& filename, const cv::Mat& mat) {
    if (mat.empty() || mat.type() != CV_8UC4) {
        return false;
    }

    int width = mat.cols;
    int height = mat.rows;
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
    file.write(reinterpret_cast<const char*>(mat.data), stride * height);
    file.close();
    return true;
}

void PrintMatInfo(const cv::Mat& mat) {
    if (mat.empty()) {
        std::cout << "[MatInfo] empty" << std::endl;
        return;
    }
    std::cout << "[MatInfo] " << mat.cols << "x" << mat.rows << ", type=" << mat.type();
    if (mat.type() == CV_8UC4) {
        cv::Vec4b p = mat.at<cv::Vec4b>(0, 0);
        std::cout << ", pixel(0,0)=(" << (int)p[0] << "," << (int)p[1] << "," << (int)p[2] << "," << (int)p[3] << ")";
    }
    std::cout << std::endl;
}

int main() {
    // ✅ 在程序最开头调用，且只调用一次
    SetProcessDPIAware();

    SetConsoleOutputCP(CP_UTF8);
    std::cout << "[Main] 启动..." << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(3));

    HWND hwnd = GetForegroundWindow();
    if (!hwnd) {
        std::cout << "[Main] ❌ 获取窗口失败" << std::endl;
        system("pause");
        return -1;
    }

    char title[256];
    GetWindowTextA(hwnd, title, sizeof(title));
    std::cout << "[Main] 窗口: " << title << std::endl;

    cv::Mat img;
    std::mutex mtx;
    std::condition_variable cv;

    WinCapture capture;

    // ---- 配置锁 ----
    lock_config lc;
    lc.downer = &cv;
    lc.downer_lock = &mtx;
    lc.uper = nullptr;
    lc.uper_lock = nullptr;
    capture.ILock_config(lc);

    // ---- 配置缓存 ----
    cache_config cc;
    cc.input = nullptr;
    cc.output = &img;
    capture.ICache_config(cc);

    // ---- 配置工作参数（cmd模式） ----
    // cmd=1: 配置HWND
    int ret = capture.IWroking_cofig(1, hwnd);
    if (ret != 0) {
        std::cout << "[Main] ❌ 注入HWND失败，错误码: " << ret << std::endl;
        system("pause");
        return -1;
    }

    // cmd=2: 配置FPS
    ret = capture.IWroking_cofig(2, 30);
    if (ret != 0) {
        std::cout << "[Main] ❌ 注入FPS失败，错误码: " << ret << std::endl;
        system("pause");
        return -1;
    }

    std::cout << "[Main] ✅ 注入HWND和FPS完成" << std::endl;

    // ---- 启动 ----
    ret = capture.ILiveing_cmd(0);
    if (ret != 0) {
        std::cout << "[Main] ❌ 初始化失败: " << ret << std::endl;
        system("pause");
        return -1;
    }

    std::cout << "[Main] 运行 3 秒..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // ---- 暂停 ----
    capture.ILiveing_cmd(1);
    PrintMatInfo(img);

    // ---- 保存 ----
    std::string path = "D:\\CodingPrograms\\Perception_Agent\\resource\\test\\test_final.bmp";
    if (SaveMatAsBmp(path, img)) {
        std::cout << "[Main] ✅ 保存: " << path << std::endl;
    } else {
        std::cout << "[Main] ❌ 保存失败" << std::endl;
    }

    // ---- 卸载 ----
    capture.ILiveing_cmd(3);
    system("pause");
    return 0;
}