#include "wincapture.h"

WinCapture::WinCapture()
    : _hwnd(nullptr)
    , _winDC(nullptr)
    , _rect{ 0, 0, 0, 0 }
    , _curDC(nullptr)
    , _inithbitmap(nullptr)
    , _curhbitmap(nullptr)
    , _isInitialized(false) {
    // 默认构造函数，所有指针置空
}

WinCapture::~WinCapture() {
    Die();
}

int WinCapture::Command(int command, void* input, void* output) {
    switch (command) {
    case 1: //初始化
        return Init(input);
    case 2: //截屏
        return Work(output);
    case 0: //生命周期结束
        return Die();
    default:
        return 010001; // 未知命令
    }
}

int WinCapture::Init(void* input) {
    // input关联为HWND，赋值到_hwnd
    _hwnd = reinterpret_cast<HWND>(input);

    // 验证有效性，无效则返回010101
    if (!IsWindow(_hwnd)) {
        return 010101;
    }

    // 获取当前屏幕dc缓存
    _winDC = GetDC(_hwnd);
    if (!_winDC) {
        return 010101;
    }

    // 创立空内存dc
    _curDC = CreateCompatibleDC(_winDC);
    if (!_curDC) {
        ReleaseDC(_hwnd, _winDC);
        _winDC = nullptr;
        return 010101;
    }

    // 把屏幕dc配置注入内存dc（通过创建兼容位图）
    _inithbitmap = CreateCompatibleBitmap(_winDC, 1, 1);
    if (!_inithbitmap) {
        DeleteDC(_curDC);
        _curDC = nullptr;
        ReleaseDC(_hwnd, _winDC);
        _winDC = nullptr;
        return 010101;
    }

    // 当前内存DC的hbitmap缓存到_inithbitmap
    SelectObject(_curDC, _inithbitmap);

    // 根据句柄获取当前窗口rect缓存到rect_，无效返回010102
    if (!GetWindowRect(_hwnd, &_rect)) {
        DeleteObject(_inithbitmap);
        _inithbitmap = nullptr;
        DeleteDC(_curDC);
        _curDC = nullptr;
        ReleaseDC(_hwnd, _winDC);
        _winDC = nullptr;
        return 010102;
    }

    // 根据rect_创立画布_curhbitmap
    int width = _rect.right - _rect.left;
    int height = _rect.bottom - _rect.top;
    _curhbitmap = CreateCompatibleBitmap(_winDC, width, height);
    if (!_curhbitmap) {
        DeleteObject(_inithbitmap);
        _inithbitmap = nullptr;
        DeleteDC(_curDC);
        _curDC = nullptr;
        ReleaseDC(_hwnd, _winDC);
        _winDC = nullptr;
        return 010102;
    }

    // 替换放入内存dc
    SelectObject(_curDC, _curhbitmap);

    _isInitialized = true;
    return 0;
}

int WinCapture::Work(void* output) {
    if (!_isInitialized || !output) {
        return 010201;
    }

    // output关联为cv::Mat，四通道
    cv::Mat* outMat = reinterpret_cast<cv::Mat*>(output);

    // 将_hwnd，printwin，渲染到内存dc
    if (!PrintWindow(_hwnd, _curDC, PW_CLIENTONLY)) {
        return 010201;
    }

    // bmp,BGRA通道，getbimap把当前画布的内容，转存到output
    int width = _rect.right - _rect.left;
    int height = _rect.bottom - _rect.top;

    // 获取位图信息
    BITMAP bmp;
    if (!GetObject(_curhbitmap, sizeof(BITMAP), &bmp)) {
        return 010201;
    }

    // 分配内存读取像素数据
    BITMAPINFO bmpInfo = { 0 };
    bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpInfo.bmiHeader.biWidth = width;
    bmpInfo.bmiHeader.biHeight = -height; // 自顶向下
    bmpInfo.bmiHeader.biPlanes = 1;
    bmpInfo.bmiHeader.biBitCount = 32;
    bmpInfo.bmiHeader.biCompression = BI_RGB;

    outMat->create(height, width, CV_8UC4);

    // 获取DIB数据
    if (!GetDIBits(_curDC, _curhbitmap, 0, height, outMat->data, &bmpInfo, DIB_RGB_COLORS)) {
        outMat->release();
        return 010201;
    }

    return 0;
}

int WinCapture::Die() {
    // _inithbitmap放回原dc
    if (_curDC && _inithbitmap) {
        SelectObject(_curDC, _inithbitmap);
    }

    // 释放_curhbitmap
    if (_curhbitmap) {
        DeleteObject(_curhbitmap);
        _curhbitmap = nullptr;
    }

    // 释放_inithbitmap
    if (_inithbitmap) {
        DeleteObject(_inithbitmap);
        _inithbitmap = nullptr;
    }

    // 释放DC
    if (_curDC) {
        DeleteDC(_curDC);
        _curDC = nullptr;
    }

    if (_winDC && _hwnd) {
        ReleaseDC(_hwnd, _winDC);
        _winDC = nullptr;
    }

    _hwnd = nullptr;
    _isInitialized = false;

    return 0;
}