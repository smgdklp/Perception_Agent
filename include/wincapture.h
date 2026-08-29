//仅暴露一个命令接口，
//commad:
//1：input传入HWND，对象创立对应虚拟DC，完成初始化
//2：output传入Mat指针，printwin到dc，最后把内容输出到对应mat,
//0：死亡，内存释放

//因为窗口无法缩放，故画布锁死，无法适应可变窗口
//对象创立后不做句柄有效性认证，炸就一起死吧（（（

//错误码：（六位正整数，前两位固定为01，中间两位由代表命令，后面两位开始递增）
// 010001 未知命令
//010101 初始化-句柄无效
//010102 初始化-获取窗口矩形失败
//010201 截屏-获取位图数据失败

#ifndef WINCAPTURE_H
#define WINCAPTURE_H

#include <windows.h>
#include <opencv2/opencv.hpp>

class WinCapture {
public:
    WinCapture();
    ~WinCapture();

    // 禁止拷贝（涉及GDI资源）
    WinCapture(const WinCapture&) = delete;
    WinCapture& operator=(const WinCapture&) = delete;

    int Command(int command, void* input, void* output);

private:
    HWND _hwnd;                 //缓存当前绑定窗口
    HDC _winDC;                 //缓存当前屏幕dc
    RECT _rect;                 //缓存当前窗口尺寸
    HDC _curDC;                 //内存DC用于窗口渲染
    HBITMAP _inithbitmap;       //初始画布槽，用于回收画布句柄
    HBITMAP _curhbitmap;        //当前画布
    bool _isInitialized;        //初始化状态标记

    int Init(void* input);
    int Work(void* output);
    int Die();
};

#endif // WINCAPTURE_H