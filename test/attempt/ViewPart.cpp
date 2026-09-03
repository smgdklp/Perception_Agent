
#include <windows.h>
#include <winuser.h>
#include <iostream>
#pragma comment(lib, "user32.lib")
#include <vector>
#include <cstring> 










//指向窗口句柄直接捆绑锁死，被动式传入一个足够大的vector然后写入数据
//画布写死，变化窗口可能爆炸

//统一command，记得管理生命周期
//统一制式，传入一个void*指针，来处理严格管数据类型
//BGRA，（x*4)*y

class Win_Capture {
private:
    //状态
    bool _is_life;
    bool _is_vaid;

    //容器
    HDC _hScreenDC;      // 屏幕DC（母版）
    HDC _hMemDC;         // 内存DC（画布容器）
    HBITMAP _Bmp;    // 画布缓存槽
    HBITMAP _oldBmp;//缓存旧画布

    //配置
    RECT _winsize;
    HWND _hwnd;

    int Set(void* input) {
        HWND hwnd = *(HWND*)input;
        if (!IsWindow(hwnd)) return 1;

        _hwnd = hwnd;
        GetWindowRect(_hwnd, &_winsize);

        _hScreenDC = GetDC(NULL);
        if (!_hScreenDC) return 2;

        _hMemDC = CreateCompatibleDC(_hScreenDC);
        if (!_hMemDC) return 3;

        int w = _winsize.right - _winsize.left;
        int h = _winsize.bottom - _winsize.top;
        _Bmp = CreateCompatibleBitmap(_hScreenDC, w, h);
        if (!_Bmp) return 4;

        _oldBmp = (HBITMAP)SelectObject(_hMemDC, _Bmp);

        _is_life = true;
        _is_vaid = true;
        return 0;
    }

    int Die() {
        // 释放dc
        if (_hMemDC) {
            SelectObject(_hMemDC, _oldBmp);
            DeleteDC(_hMemDC);
            _hMemDC = NULL;
        }
        if (_hScreenDC) {
            ReleaseDC(NULL, _hScreenDC);
            _hScreenDC = NULL;
        }
        //旧画布塞回去，释放_Bmp
        if (_Bmp) {
            DeleteObject(_Bmp);
            _Bmp = NULL;
        }
        _is_life = false;
        _is_vaid = false;
        return 0;
    }

    int Work(void* output) {
        if (!_is_vaid || !_is_life) return 5;

        PrintWindow(_hwnd, _hMemDC, PW_RENDERFULLCONTENT);

        int w = _winsize.right - _winsize.left;
        int h = _winsize.bottom - _winsize.top;
        int stride = ((w * 32 + 31) / 32) * 4;

        std::vector<BYTE>& outVec = *(std::vector<BYTE>*)output;
        outVec.resize(h * stride);

        BITMAP bm;
        GetObject(_Bmp, sizeof(BITMAP), &bm);

        std::vector<BYTE> fullData(bm.bmWidth * bm.bmHeight * 4);
        if (!GetBitmapBits(_Bmp, fullData.size(), fullData.data())) return 6;

        for (int y = 0; y < h; y++) {
            int srcOffset = y * bm.bmWidth * 4;
            int dstOffset = y * stride;
            memcpy(outVec.data() + dstOffset, fullData.data() + srcOffset, w * 4);
        }

        return 0;
    }

public:
    Win_Capture() : _is_life(false), _is_vaid(false), _hwnd(NULL),
        _hScreenDC(NULL), _hMemDC(NULL), _Bmp(NULL), _oldBmp(NULL) {
        ZeroMemory(&_winsize, sizeof(RECT));
    }

    ~Win_Capture() {
        if (_is_life) Die();
    }

    int Command(int command, void* input, void* output) {
        switch (command) {
        case 0: // Die
            return Die();
        case 1: // Set，指向窗口直接捆绑锁死
            return Set(input);
        case 3: //工作
            return Work(output);
        default:
            return -831;
        }
    }
};

/*
错误码说明：
0   - 正常
1   - 句柄无效
2   - 获取屏幕DC失败
3   - 创建内存DC失败
4   - 创建画布失败
5   - 对象未初始化或已销毁
6   - 获取位图数据失败
-831- 未知命令
*/