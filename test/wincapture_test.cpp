#include "wincapture.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <windows.h>

// Windows GDI 保存 BMP（不依赖 OpenCV imwrite）
bool SaveMatAsBmp(const std::string& filename, const cv::Mat& mat) {
    if (mat.empty() || mat.type() != CV_8UC4) {
        std::cout << "[Error] SaveMatAsBmp: 不支持的 Mat 格式！" << std::endl;
        return false;
    }

    int width = mat.cols;
    int height = mat.rows;
    int stride = width * 4;

    BITMAPFILEHEADER fileHeader = { 0 };
    fileHeader.bfType = 0x4D42;
    fileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + stride * height;
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    BITMAPINFOHEADER infoHeader = { 0 };
    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = width;
    infoHeader.biHeight = -height;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 32;
    infoHeader.biCompression = BI_RGB;
    infoHeader.biSizeImage = 0;

    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "[Error] 无法创建文件: " << filename << std::endl;
        return false;
    }

    file.write(reinterpret_cast<const char*>(&fileHeader), sizeof(BITMAPFILEHEADER));
    if (!file.good()) {
        std::cout << "[Error] 写入 BITMAPFILEHEADER 失败！" << std::endl;
        return false;
    }

    file.write(reinterpret_cast<const char*>(&infoHeader), sizeof(BITMAPINFOHEADER));
    if (!file.good()) {
        std::cout << "[Error] 写入 BITMAPINFOHEADER 失败！" << std::endl;
        return false;
    }

    const char* pixelData = reinterpret_cast<const char*>(mat.data);
    file.write(pixelData, stride * height);
    if (!file.good()) {
        std::cout << "[Error] 写入像素数据失败！" << std::endl;
        return false;
    }

    file.close();
    return true;
}

// 获取当前时间戳字符串
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

// 🆕 获取窗口标题（辅助函数）
std::string GetWindowTitle(HWND hwnd) {
    char windowTitle[256] = { 0 };
    GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle));
    return std::string(windowTitle);
}

int main() {
    // 创立一个cv::Mat img四通道，预留1980*1020*4
    cv::Mat img(1020, 1980, CV_8UC4);
    std::cout << "[Main] 创建Mat成功，尺寸: " << img.cols << "x" << img.rows
        << ", 通道数: " << img.channels() << std::endl;

    // 等待2秒
    std::cout << "[Main] 等待2秒..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 获取当前主屏幕最上层窗口句柄
    HWND hightest_hwnd = GetForegroundWindow();
    if (!hightest_hwnd) {
        std::cout << "[Main] 获取最上层窗口失败！" << std::endl;
        return -1;
    }

    // 打印对应窗口的标签头
    std::string title = GetWindowTitle(hightest_hwnd);
    std::cout << "[Main] 获取到最上层窗口句柄: 0x" << std::hex << hightest_hwnd
        << std::dec << ", 标题: \"" << title << "\"" << std::endl;

    // 创立wincapture，把获得hightest_hwnd注入
    WinCapture capture;
    int initResult = capture.Command(1, hightest_hwnd, nullptr);
    if (initResult != 0) {
        std::cout << "[Main] 初始化失败，错误码: " << initResult << std::endl;
        return -1;
    }
    std::cout << "[Main] WinCapture初始化成功！" << std::endl;

    // 定义Catch函数（用 Windows GDI 保存 BMP）
    auto Catch = [&](cv::Mat& img, WinCapture& capture, HWND hwnd) -> bool {
        // 🆕 1. 先检验句柄有效性
        if (!IsWindow(hwnd)) {
            std::cout << "[Catch] 窗口句柄已失效！(0x" << std::hex << hwnd << std::dec << ")" << std::endl;
            return false;
        }

        // 🆕 2. 打印当前窗口标题
        std::string currentTitle = GetWindowTitle(hwnd);
        std::cout << "[Catch] 当前窗口标题: \"" << currentTitle << "\"" << std::endl;

        // 3. work保存到img
        int workResult = capture.Command(2, nullptr, &img);
        if (workResult != 0) {
            std::cout << "[Catch] 截屏失败，错误码: " << workResult << std::endl;
            return false;
        }

        // 4. 生成时间戳文件名
        std::string timestamp = GetTimestamp();
        std::string savePath = "D:\\CodingPrograms\\Perception_Agent\\resource\\test\\" + timestamp + ".bmp";

        // 5. 用 Windows GDI 保存 BMP
        if (!SaveMatAsBmp(savePath, img)) {
            std::cout << "[Catch] 保存图片失败: " << savePath << std::endl;
            return false;
        }

        std::cout << "[Catch] 截图保存成功: " << savePath << std::endl;
        return true;
        };

    // 执行10次Catch()
    std::cout << "[Main] 开始执行10次截图..." << std::endl;
    int successCount = 0;
    for (int i = 1; i <= 10; ++i) {
        std::cout << "[Main] 第" << i << "次截图开始..." << std::endl;

        // 🆕 传入 hightest_hwnd 用于检验
        if (Catch(img, capture, hightest_hwnd)) {
            successCount++;
        }

        // 🆕 间隔修改为 1 秒（原来是 500ms）
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << "[Main] 截图完成，成功: " << successCount << "/10" << std::endl;

    // wincapture直接die
    int dieResult = capture.Command(0, nullptr, nullptr);
    std::cout << "[Main] WinCapture销毁，结果: " << dieResult << std::endl;

    std::cout << "[Main] 程序结束！" << std::endl;
    return 0;
}