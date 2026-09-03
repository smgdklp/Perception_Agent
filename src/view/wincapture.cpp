#include "wincapture.h"
#include <iostream>

WinCapture::WinCapture()
    : _downer(nullptr)
    , _downer_lock(nullptr)
    , _uper(nullptr)
    , _uper_lock(nullptr)
    , _output(nullptr)
    , _hwnd(nullptr)
    , _intervalMs(33)                       //默认约30fps
    , _winDC(nullptr)
    , _curDC(nullptr)
    , _initmap(nullptr)
    , _curmap(nullptr)
    , _iswork(false)
    , _islife(true)
    , _fps(30) {
    _info = { 0 };
    _info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    _info.bmiHeader.biPlanes = 1;
    _info.bmiHeader.biBitCount = 32;
    _info.bmiHeader.biCompression = BI_RGB;
}

WinCapture::~WinCapture() {
    if (_islife) {
        OnUnload();
    }
}

void WinCapture::Timer() {
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - _lasttime);
    int sleepMs = _intervalMs - static_cast<int>(elapsed.count());
    if (sleepMs > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
    }
    _lasttime = std::chrono::steady_clock::now();
}

int WinCapture::Capture() {
    //检查_hwnd句柄有效性
    if (!IsWindow(_hwnd)) {
        return 011001;
    }

    //获取_hwnd对应窗口的RECT
    if (!GetWindowRect(_hwnd, &_size)) {
        return 011005;
    }

    int width = _size.right - _size.left;
    int height = _size.bottom - _size.top;
    if (width <= 0 || height <= 0) {
        return 011006;
    }

    //_hwnd printwin到内存dc_curDC
    if (!PrintWindow(_hwnd, _curDC, PW_CLIENTONLY)) {
        return 011201;
    }

    return 0;
}

int WinCapture::Save() {
    if (!_output) {
        return 011204;
    }

    int width = _size.right - _size.left;
    int height = _size.bottom - _size.top;

    //根据Rect填充info
    _info.bmiHeader.biWidth = width;
    _info.bmiHeader.biHeight = -height;     //自顶向下

    _output->create(height, width, CV_8UC4);

    //GetDIBits直接把像素信息缓存到_output的cv::Mat对象
    if (!GetDIBits(_curDC, _curmap, 0, height, _output->data, &_info, DIB_RGB_COLORS)) {
        _output->release();
        return 011202;
    }

    return 0;
}

void WinCapture::Work() {
    while (_islife) {
        if (_iswork) {
            Timer();

            int ret = Capture();
            if (ret != 0) {
                std::cout << "[WinCapture] Capture失败: " << ret << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            ret = Save();
            if (ret != 0) {
                std::cout << "[WinCapture] Save失败: " << ret << std::endl;
                continue;
            }

            //广播通知消费者
            if (_downer && _downer_lock) {
                std::lock_guard<std::mutex> lock(*_downer_lock);
                _downer->notify_one();
            }
        } else {
            //暂停状态，等待唤醒
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

int WinCapture::MainWork() {
    if (_workThread.joinable()) {
        return 010009;
    }

    try {
        _workThread = std::thread(&WinCapture::Work, this);
    } catch (...) {
        return 010007;
    }

    return 0;
}

int WinCapture::OnInit() {
    //检查配置对象是否有效
    if (!_hwnd) {
        return 011001;
    }
    if (!_output) {
        return 010002;
    }

    //_time设置为当前时间
    _lasttime = std::chrono::steady_clock::now();

    //获取当前主屏幕的DC缓存到WinDC _winDC
    _winDC = GetDC(_hwnd);
    if (!_winDC) {
        return 011002;
    }

    //创建内存DC_curDC
    _curDC = CreateCompatibleDC(_winDC);
    if (!_curDC) {
        ReleaseDC(_hwnd, _winDC);
        _winDC = nullptr;
        return 011003;
    }

    //_curDC内画布保存到_initmap
    _initmap = CreateCompatibleBitmap(_winDC, 1, 1);
    if (!_initmap) {
        DeleteDC(_curDC);
        _curDC = nullptr;
        ReleaseDC(_hwnd, _winDC);
        _winDC = nullptr;
        return 011004;
    }
    SelectObject(_curDC, _initmap);

    //创立画布_curmap，尺寸1920*1080，替换加入_curDC
    _curmap = CreateCompatibleBitmap(_winDC, 1920, 1080);
    if (!_curmap) {
        DeleteObject(_initmap);
        _initmap = nullptr;
        DeleteDC(_curDC);
        _curDC = nullptr;
        ReleaseDC(_hwnd, _winDC);
        _winDC = nullptr;
        return 011004;
    }
    SelectObject(_curDC, _curmap);

    _iswork = true;
    _islife = true;

    return MainWork();
}

int WinCapture::OnPause() {
    if (!_iswork) {
        return 011301;
    }
    _iswork = false;
    return 0;
}

int WinCapture::OnResume() {
    if (_iswork) {
        return 011302;
    }
    _iswork = true;
    _lasttime = std::chrono::steady_clock::now();
    return 0;
}

int WinCapture::OnUnload() {
    _islife = false;
    _iswork = false;

    if (_workThread.joinable()) {
        try {
            _workThread.join();
        } catch (...) {
            return 011402;
        }
    }

    //将_initmap放回内存DC
    if (_curDC && _initmap) {
        SelectObject(_curDC, _initmap);
    }

    //释放内存DC
    if (_curmap) {
        DeleteObject(_curmap);
        _curmap = nullptr;
    }

    if (_initmap) {
        DeleteObject(_initmap);
        _initmap = nullptr;
    }

    if (_curDC) {
        DeleteDC(_curDC);
        _curDC = nullptr;
    }

    if (_winDC && _hwnd) {
        ReleaseDC(_hwnd, _winDC);
        _winDC = nullptr;
    }

    return 0;
}

//配置槽函数实现
int WinCapture::ILock_config(lock_config config) {
    if (config.downer) {
        _downer = config.downer;
    }
    if (config.downer_lock) {
        _downer_lock = config.downer_lock;
    }
    if (config.uper) {
        _uper = config.uper;
    }
    if (config.uper_lock) {
        _uper_lock = config.uper_lock;
    }
    return 0;
}

int WinCapture::ICache_config(cache_config config) {
    if (config.output) {
        _output = reinterpret_cast<cv::Mat*>(config.output);
    }
    if (config.input) {
        //input预留，暂不处理
    }
    return 0;
}

int WinCapture::IWroking_cofig(working_config config) {
    if (config.type == ConfigType::HWND) {
        //传入HWND
        auto* p = std::any_cast<HWND>(&config.info);
        if (p) {
            _hwnd = *p;
        } else {
            return 010004;
        }
    } else if (config.type == ConfigType::FPS) {
        //传入fps，转化为ms保存
        auto* p = std::any_cast<int>(&config.info);
        if (p && *p > 0) {
            _fps = *p;
            _intervalMs = 1000 / _fps;
        } else {
            return 010010;
        }
    } else {
        return 010004;
    }
    return 0;
}

int WinCapture::IWorking_cmd(int cmd, void* input, void* output) {
    //泛用指令接口，预留扩展
    //cmd:
    switch (cmd) {
   
    default:
        return 010001;
    }
}