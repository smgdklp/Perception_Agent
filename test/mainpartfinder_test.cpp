#include "wincapture.h"
#include "mainpartfinder.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <windows.h>
#include <opencv2/opencv.hpp>

// 设置控制台输出为UTF-8
void SetConsoleUTF8() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}

// 在Mat上绘制矩形并保存
void DrawRectAndSave(const cv::Mat& src, const RECT& rect, const std::string& savePath) {
    cv::Mat dst = src.clone();
    
    // 绘制矩形（绿色边框，粗细2像素）
    cv::rectangle(dst, 
                  cv::Point(rect.left, rect.top),
                  cv::Point(rect.right, rect.bottom),
                  cv::Scalar(0, 255, 0, 255),  // BGRA绿色
                  2);
    
    cv::imwrite(savePath, dst);
    std::cout << "[Main] 标注图片已保存: " << savePath << std::endl;
}

int main() {
    // 设置DPI感知
    SetProcessDPIAware();
    
    // 设置控制台UTF-8
    SetConsoleUTF8();
    
    std::cout << "========================================" << std::endl;
    std::cout << "   WinCapture + MainPartFinder 测试程序" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // ---- 创建完整锁依赖 ----
    std::mutex mtx;
    std::condition_variable cv;
    
    // ---- 创建中间池子 ----
    cv::Mat img;
    RECT resultRect = {0, 0, 0, 0};
    
    // ---- 创建WinCapture ----
    WinCapture* capture = new WinCapture();
    std::cout << "[Main] WinCapture 创建成功" << std::endl;
    
    // ---- 创建MainPartFinder ----
    MainPartFinder* finder = new MainPartFinder();
    std::cout << "[Main] MainPartFinder 创建成功" << std::endl;
    
    // ---- 配置WinCapture ----
    // 配置锁（生产者通知消费者）
    lock_config lockCfg;
    lockCfg.downer = &cv;
    lockCfg.downer_lock = &mtx;
    lockCfg.uper = nullptr;
    lockCfg.uper_lock = nullptr;
    capture->ILock_config(lockCfg);
    
    // 配置缓存（输出到img）
    cache_config cacheCfg;
    cacheCfg.input = nullptr;
    cacheCfg.output = &img;
    capture->ICache_config(cacheCfg);
    
    std::cout << "[Main] WinCapture 配置完成" << std::endl;
    
    // ---- 配置MainPartFinder ----
    // 配置锁（接收生产者通知）
    lock_config finderLockCfg;
    finderLockCfg.downer = nullptr;
    finderLockCfg.downer_lock = nullptr;
    finderLockCfg.uper = &cv;
    finderLockCfg.uper_lock = &mtx;
    finder->ILock_config(finderLockCfg);
    
    // 配置缓存（输入从img读取，输出到resultRect）
    cache_config finderCacheCfg;
    finderCacheCfg.input = &img;
    finderCacheCfg.output = &resultRect;
    finder->ICache_config(finderCacheCfg);
    
    
    // 配置工作参数（使用pair传参）
    working_config workCfg;
    workCfg.type = ConfigType::INT;
    // 配置调试路径
workCfg.type = ConfigType::STRING;
workCfg.info = std::string("D:\\CodingPrograms\\Perception_Agent\\resource\\test");
finder->IWroking_cofig(workCfg);
    workCfg.info = std::make_pair<std::string, int>("light", 100);
    finder->IWroking_cofig(workCfg);
    
    workCfg.info = std::make_pair<std::string, int>("num", 50);
    finder->IWroking_cofig(workCfg);
    
    std::cout << "[Main] MainPartFinder 配置完成" << std::endl;
    
    // ---- 等待3秒 ----
    std::cout << "[Main] 等待3秒，请切换到目标窗口..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // ---- 获取最上层窗口句柄 ----
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) {
        std::cout << "[Main] ❌ 获取最上层窗口失败！" << std::endl;
        delete capture;
        delete finder;
        system("pause");
        return -1;
    }
    
    char windowTitle[256];
    GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle));
    std::cout << "[Main] ✅ 获取到窗口: 0x" << std::hex << hwnd 
              << std::dec << " \"" << windowTitle << "\"" << std::endl;
    
    // ---- 注入HWND到WinCapture ----
    workCfg.type = ConfigType::HWND;
    workCfg.info = hwnd;
    int ret = capture->IWroking_cofig(workCfg);
    if (ret != 0) {
        std::cout << "[Main] ❌ 注入HWND失败，错误码: " << ret << std::endl;
        delete capture;
        delete finder;
        system("pause");
        return -1;
    }
    
    // 配置FPS
    workCfg.type = ConfigType::FPS;
    workCfg.info = 30;
    capture->IWroking_cofig(workCfg);
    std::cout << "[Main] ✅ 注入HWND和FPS完成" << std::endl;
    
    // ---- 启动WinCapture ----
    std::cout << "[Main] 启动 WinCapture..." << std::endl;
    ret = capture->ILiveing_cmd(0);  // OnInit
    if (ret != 0) {
        std::cout << "[Main] ❌ WinCapture启动失败，错误码: " << ret << std::endl;
        delete capture;
        delete finder;
        system("pause");
        return -1;
    }
    std::cout << "[Main] ✅ WinCapture 已启动" << std::endl;
    
    // ---- 启动MainPartFinder ----
    std::cout << "[Main] 启动 MainPartFinder..." << std::endl;
    ret = finder->ILiveing_cmd(0);  // OnInit
    if (ret != 0) {
        std::cout << "[Main] ❌ MainPartFinder启动失败，错误码: " << ret << std::endl;
        capture->ILiveing_cmd(3);
        delete capture;
        delete finder;
        system("pause");
        return -1;
    }
    std::cout << "[Main] ✅ MainPartFinder 已启动" << std::endl;
    
    // ---- 等待1秒让生产者生成帧 ----
    std::cout << "[Main] 等待1秒生成帧数据..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // ---- 执行一次检测 ----
    std::cout << "[Main] 执行一次主区域检测..." << std::endl;
    ret = finder->IWorking_cmd(1, nullptr, nullptr);
    if (ret != 0) {
        std::cout << "[Main] ❌ 检测执行失败，错误码: " << ret << std::endl;
    } else {
        // 等待异步完成（简单sleep）
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::cout << "[Main] ✅ 检测完成" << std::endl;
        std::cout << "[Main] 结果 RECT: left=" << resultRect.left 
                  << ", top=" << resultRect.top
                  << ", right=" << resultRect.right
                  << ", bottom=" << resultRect.bottom
                  << " (宽=" << (resultRect.right - resultRect.left)
                  << ", 高=" << (resultRect.bottom - resultRect.top) << ")"
                  << std::endl;
    }
    
    // ---- 暂停所有工作 ----
    std::cout << "[Main] 暂停 WinCapture 和 MainPartFinder..." << std::endl;
    capture->ILiveing_cmd(1);  // OnPause
    finder->ILiveing_cmd(1);   // OnPause
    std::cout << "[Main] ✅ 已暂停" << std::endl;
    
    // ---- 保存原图 ----
    std::string path1 = "D:\\CodingPrograms\\Perception_Agent\\resource\\test\\test1.bmp";
    if (!img.empty()) {
        if (cv::imwrite(path1, img)) {
            std::cout << "[Main] ✅ 原图保存成功: " << path1 << std::endl;
        } else {
            std::cout << "[Main] ❌ 原图保存失败: " << path1 << std::endl;
        }
    } else {
        std::cout << "[Main] ❌ img为空，无法保存" << std::endl;
    }
    
    // ---- 保存标注图 ----
    if (!img.empty() && resultRect.right > resultRect.left) {
        std::string path2 = "D:\\CodingPrograms\\Perception_Agent\\resource\\test\\test2.bmp";
        DrawRectAndSave(img, resultRect, path2);
    } else {
        std::cout << "[Main] ⚠️ 矩形无效或图片为空，跳过标注" << std::endl;
    }
    
    // ---- 清理资源 ----
    std::cout << "[Main] 清理资源..." << std::endl;
    capture->ILiveing_cmd(3);  // OnUnload
    finder->ILiveing_cmd(3);   // OnUnload
    
    delete capture;
    delete finder;
    std::cout << "[Main] ✅ 资源已释放" << std::endl;
    
    // ---- 等待按键 ----
    std::cout << "\n========================================" << std::endl;
    std::cout << "按 Enter 键退出... (或输入 'e' 再按 Enter 重新运行)" << std::endl;
    std::cout << "========================================" << std::endl;
    
    char input = getchar();
    if (input == 'e' || input == 'E') {
        std::cout << "[Main] 重新运行... (请重新编译执行)" << std::endl;
    }
    
    return 0;
}