#include <windows.h>
#include <winuser.h>
#include <iostream>
#pragma comment(lib, "user32.lib")
//维护当前窗口的简单状态机

//enum class WinState : uint32_t {
//	None = 0x0000,
//	Valid = 0x0001,
//	Show = 0x0002,
//	Life = 0x0004,
//};
//不锁了了！！！！！！记得out是外面类成员应用自己管自己锁！！！！
class Win_Capture {
    // 状态值
private:
    HWND _hwnd;
    bool _is_vail;
    bool _is_life;

    // 补充容器成员
    HDC _hScreenDC;      // 屏幕DC（母版）
    HDC _hMemDC;         // 内存DC（画布容器）
    HBITMAP _hOldBmp;    // 旧画布（用于回收）
    HBITMAP _hCurBmp;    // 当前画布（_cur_p）
    HBITMAP _hNewBmp;    // 新画布（_new_p，临时用）
    RECT _rcWnd;         // 窗口矩形（缓存尺寸）

    int Isvaild(HWND hwnd) {
        // 判断句柄有效性
        return (hwnd != NULL && IsWindow(hwnd)) ? 1 : 0;
    }

    int Life() {
        _is_life = true;
        // 创立一份dc,复制屏幕的
        _hScreenDC = GetDC(NULL);
        _hMemDC = CreateCompatibleDC(_hScreenDC);

        // 然后三缓存画布初始化，先缓存旧画布_init_p方便回收
        // 先创建一个占位画布（1x1像素，避免空DC问题）
        _hCurBmp = CreateCompatibleBitmap(_hScreenDC, 1, 1);
        _hOldBmp = (HBITMAP)SelectObject(_hMemDC, _hCurBmp);
        _hNewBmp = NULL;

        // _cur_p存当前在dc画布，_new_p缓存要创立的画布
        // _hCurBmp 即为 _cur_p
        // _hNewBmp 即为 _new_p
        return 0;
    }

    int Die() {
        // 释放dc
        // 把旧画布塞回去，释放取出来的画布
        if (_hMemDC) {
            SelectObject(_hMemDC, _hOldBmp);  // 恢复旧画布
            DeleteDC(_hMemDC);
            _hMemDC = NULL;
        }
        if (_hScreenDC) {
            ReleaseDC(NULL, _hScreenDC);
            _hScreenDC = NULL;
        }
        if (_hCurBmp) {
            DeleteObject(_hCurBmp);
            _hCurBmp = NULL;
        }
        if (_hNewBmp) {
            DeleteObject(_hNewBmp);
            _hNewBmp = NULL;
        }
        _is_life = false;
        return 0;
    }

    int SetWindowHandle(HWND hwnd) {
        _hwnd = hwnd;
        _is_vail = (Isvaild(hwnd) != 0);
        return _is_vail ? 1 : 0;
    }

    int CatchWindow(void* output) {

        if (!_is_vail || !_is_life) return -1;
        if (output){
            // 通过句柄获取rect
            GetWindowRect(_hwnd, &_rcWnd);
        int newW = _rcWnd.right - _rcWnd.left;
        int newH = _rcWnd.bottom - _rcWnd.top;
        if (newW <= 0 || newH <= 0) return -2;

        // 判断是否和_cur_p尺寸一样
        BITMAP bm;
        GetObject(_hCurBmp, sizeof(BITMAP), &bm);
        int curW = bm.bmWidth;
        int curH = bm.bmHeight;

        if (curW == newW && curH == newH) {
            // 一样：渲染到dc,然后结果存到output
            PrintWindow(_hwnd, _hMemDC, PW_RENDERFULLCONTENT);
        }
        else {
            // 不一样：_new_p创立新画布，然后替换塞进去，释放旧画布，再把_cur_p变成当前画布
            _hNewBmp = CreateCompatibleBitmap(_hScreenDC, newW, newH);
            if (!_hNewBmp) return -3;

            // 替换画布：先取出当前画布（此时DC中换成了新画布）
            HBITMAP hOldInDC = (HBITMAP)SelectObject(_hMemDC, _hNewBmp);
            // 如果旧画布就是 _hCurBmp，删除它；否则删除取出来的
            if (hOldInDC == _hCurBmp) {
                DeleteObject(_hCurBmp);
            }
            else {
                DeleteObject(hOldInDC);
            }
            _hCurBmp = _hNewBmp;         // _cur_p变成当前画布
            _hNewBmp = NULL;

            // 渲染到dc,然后结果存到output
            PrintWindow(_hwnd, _hMemDC, PW_RENDERFULLCONTENT);
        }

        // 结果存到output（假设output是HBITMAP*类型）

        *(HBITMAP*)output = _hCurBmp;  // 返回当前画布句柄
    }
        return 0;
    }

public:
    Win_Capture() : _hwnd(NULL), _is_vail(false), _is_life(false),
        _hScreenDC(NULL), _hMemDC(NULL),
        _hOldBmp(NULL), _hCurBmp(NULL), _hNewBmp(NULL) {
        ZeroMemory(&_rcWnd, sizeof(RECT));
      
    }

    ~Win_Capture() {
        if (_is_life) Die();
    }

    int Command(int command, void* input, void* output) {
        switch (command) {
        case 0: // Die
            return Die();
        case 1: // Life
            return Life();
        case 2: // Set Window Handle
            return SetWindowHandle(*(HWND*)input);
        case 3: // Catch Window
            return CatchWindow(output);

        default:
            return -831;
        }
    }
};
