#include "wincapture.h"
#include <iostream>

WinCapture::WinCapture()
    : _downer(nullptr)
    , _downer_lock(nullptr)
    , _uper(nullptr)
    , _uper_lock(nullptr)
    , _output(nullptr)
    , _hwnd(nullptr)
    , _intervalMs(33)
    , _winDC(nullptr)
    , _curDC(nullptr)
    , _initmap(nullptr)
    , _curmap(nullptr)
    , _iswork(false)
    , _islife(true)
    , _fps(30)
    , _dpiX(96)
    , _dpiY(96)
    , _scaleX(1.0)
    , _scaleY(1.0) {
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
    if (!IsWindow(_hwnd)) {
        return 11101;
    }

    //获取客户区尺寸（实际内容区域）
    RECT clientRect;
    if (!GetClientRect(_hwnd, &clientRect)) {
        return 11105;
    }

    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;
    if (width <= 0 || height <= 0) {
        return 11106;
    }

    // 存储客户区尺寸（用于保存）
    _size = clientRect;

    if (_curmap) {
        BITMAP bmp;
        GetObject(_curmap, sizeof(BITMAP), &bmp);
        if (bmp.bmWidth != width || bmp.bmHeight != height) {
            SelectObject(_curDC, _initmap);
            DeleteObject(_curmap);
            _curmap = nullptr;
        }
    }

    if (!_curmap) {
        _curmap = CreateCompatibleBitmap(_winDC, width, height);
        if (!_curmap) {
            return 11104;
        }
        SelectObject(_curDC, _curmap);
    }

    if (!PrintWindow(_hwnd, _curDC, PW_CLIENTONLY)) {
        return 11201;
    }

    return 0;
}

int WinCapture::Save() {
    if (!_output) {
        return 11204;
    }

    BITMAP bmp;
    if (!GetObject(_curmap, sizeof(BITMAP), &bmp)) {
        return 11202;
    }

    int width = bmp.bmWidth;
    int height = bmp.bmHeight;
    if (width <= 0 || height <= 0) {
        return 11106;
    }

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = 0;

    _output->create(height, width, CV_8UC4);
    if (_output->empty()) {
        return 11202;
    }

    HDC hdcMem = CreateCompatibleDC(nullptr);
    if (!hdcMem) {
        _output->release();
        return 11202;
    }

    SelectObject(hdcMem, _curmap);

    int result = GetDIBits(hdcMem, _curmap, 0, height, _output->data, &bmi, DIB_RGB_COLORS);

    DeleteDC(hdcMem);

    if (result != height) {
        _output->release();
        return 11202;
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

            if (_downer && _downer_lock) {
                std::lock_guard<std::mutex> lock(*_downer_lock);
                _downer->notify_one();
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

int WinCapture::MainWork() {
    if (_workThread.joinable()) {
        return 11009;
    }

    try {
        _workThread = std::thread(&WinCapture::Work, this);
    } catch (...) {
        return 11007;
    }

    return 0;
}

int WinCapture::OnInit() {
    if (!_hwnd) {
        return 11101;
    }
    if (!_output) {
        return 11002;
    }

    // ✅ 获取客户区尺寸
    RECT clientRect;
    if (!GetClientRect(_hwnd, &clientRect)) {
        return 11105;
    }

    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;
    if (width <= 0 || height <= 0) {
        return 11106;
    }

    _size = clientRect;

    _lasttime = std::chrono::steady_clock::now();

    _winDC = GetDC(_hwnd);
    if (!_winDC) {
        return 11102;
    }

    _curDC = CreateCompatibleDC(_winDC);
    if (!_curDC) {
        ReleaseDC(_hwnd, _winDC);
        _winDC = nullptr;
        return 11103;
    }

    _initmap = CreateCompatibleBitmap(_winDC, 1, 1);
    if (!_initmap) {
        DeleteDC(_curDC);
        _curDC = nullptr;
        ReleaseDC(_hwnd, _winDC);
        _winDC = nullptr;
        return 11104;
    }
    SelectObject(_curDC, _initmap);

    _curmap = CreateCompatibleBitmap(_winDC, width, height);
    if (!_curmap) {
        DeleteObject(_initmap);
        _initmap = nullptr;
        DeleteDC(_curDC);
        _curDC = nullptr;
        ReleaseDC(_hwnd, _winDC);
        _winDC = nullptr;
        return 11104;
    }
    SelectObject(_curDC, _curmap);

    std::cout << "[WinCapture] 客户区尺寸: " << width << "x" << height << std::endl;

    _iswork = true;
    _islife = true;

    return MainWork();
}

int WinCapture::OnPause() {
    if (!_iswork) {
        return 11301;
    }
    _iswork = false;
    return 0;
}

int WinCapture::OnResume() {
    if (_iswork) {
        return 11302;
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
            return 11402;
        }
    }

    if (_curDC && _initmap) {
        SelectObject(_curDC, _initmap);
    }

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
    return 0;
}

int WinCapture::IWroking_cofig(working_config config) {
    if (config.type == ConfigType::HWND) {
        auto* p = std::any_cast<HWND>(&config.info);
        if (p) {
            _hwnd = *p;
        } else {
            return 11004;
        }
    } else if (config.type == ConfigType::FPS) {
        auto* p = std::any_cast<int>(&config.info);
        if (p && *p > 0) {
            _fps = *p;
            _intervalMs = 1000 / _fps;
        } else {
            return 11010;
        }
    } else {
        return 11004;
    }
    return 0;
}

int WinCapture::IWorking_cmd(int cmd, void* input, void* output) {
    switch (cmd) {
    case 1: {
        if (output == nullptr) {
            return 11003;
        }
        HBITMAP* hBmpOut = reinterpret_cast<HBITMAP*>(output);
        *hBmpOut = _curmap;
        return 0;
    }
    case 2: {
        if (output == nullptr) {
            return 11003;
        }
        RECT* rectOut = reinterpret_cast<RECT*>(output);
        *rectOut = _size;
        return 0;
    }
    default:
        return 11001;
    }
}