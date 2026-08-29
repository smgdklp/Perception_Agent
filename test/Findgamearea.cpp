#include <windows.h>
#include <winuser.h>
#include <iostream>
#pragma comment(lib, "user32.lib")
#include <opencv2/opencv.hpp>
using namespace cv;
using namespace std;
#include <vector>



class MainPartFinder {
private:
    int sizex;
    int sizey;
    std::vector<BYTE> _input_cache1;
    std::vector<BYTE> _input_cache2;
    std::vector<BYTE> _light_cache1;
    std::vector<BYTE> _light_cache2;
    std::vector<BYTE> _fin_cache;
    std::vector<int> _supx_cache;
    std::vector<int> _supy_cache;
    int _maxx;
    int _minx;
    int _miny;
    int _maxy;
    int _lightgrad;
    int _threshold;
    RECT _cacheresult;

    int Die() {
        return 0;
    }

    int Set(void* input) {
        return 0;
    }

    //输入两张拼接图vector<byte>(x * 4) * (y * 2)
    //输出RECT
    int LightGrad() {
        cv::Mat img1(sizex, sizey, CV_8UC4, _input_cache1.data());
        cv::Mat img2(sizex, sizey, CV_8UC4, _input_cache2.data());

        cv::Mat gray1, gray2;
        cv::cvtColor(img1, gray1, cv::COLOR_BGRA2GRAY);
        cv::cvtColor(img2, gray2, cv::COLOR_BGRA2GRAY);

        _light_cache1.resize(sizex * sizey);
        _light_cache2.resize(sizex * sizey);
        memcpy(_light_cache1.data(), gray1.data(), sizex * sizey);
        memcpy(_light_cache2.data(), gray2.data(), sizex * sizey);

        _fin_cache.resize(sizex * sizey);
        cv::Mat diff(sizex, sizey, CV_8U);
        cv::absdiff(gray1, gray2, diff);

        cv::Mat mask(sizex, sizey, CV_8U);
        cv::threshold(diff, mask, _lightgrad, 1, cv::THRESH_BINARY);

        memcpy(_fin_cache.data(), mask.data(), sizex * sizey);
        return 0;
    }

    int FindGrad() {
        _supx_cache.resize(sizex, 0);
        _supy_cache.resize(sizey, 0);

        cv::Mat fin(sizex, sizey, CV_8U, _fin_cache.data());

        for (int x = 0; x < sizex; x++) {
            cv::Mat col = fin.row(x);
            _supx_cache[x] = cv::sum(col)[0];
        }

        for (int y = 0; y < sizey; y++) {
            cv::Mat row = fin.col(y);
            _supy_cache[y] = cv::sum(row)[0];
        }

        _minx = -1;
        _maxx = -1;
        for (int x = 0; x < sizex; x++) {
            if (_supx_cache[x] > _threshold) {
                if (_minx == -1) _minx = x;
                _maxx = x;
            }
        }

        _miny = -1;
        _maxy = -1;
        for (int y = 0; y < sizey; y++) {
            if (_supy_cache[y] > _threshold) {
                if (_miny == -1) _miny = y;
                _maxy = y;
            }
        }

        if (_minx == -1 || _miny == -1) {
            _cacheresult = { 0, 0, 0, 0 };
        }
        else {
            _cacheresult.left = _minx;
            _cacheresult.top = _miny;
            _cacheresult.right = _maxx + 1;
            _cacheresult.bottom = _maxy + 1;
        }
        return 0;
    }

    int Work(void* input, void* output) {
        //inputstd::vector<BYTE>,(x * 4) * (y * 2)拷贝到模式是BGRA
        std::vector<BYTE>* inputData = (std::vector<BYTE>*)input;
        int totalSize = inputData->size();
        int halfSize = totalSize / 2;

        //先提取x,y缓存到sizex,sizey
        sizex = 1980;
        sizey = 1080;

        //把上下拼接的两张图缓存到(x * 4) * y, _input_cache1,(x * 4) * y, _input_cache2
        _input_cache1.resize(halfSize);
        _input_cache2.resize(halfSize);
        memcpy(_input_cache1.data(), inputData->data(), halfSize);
        memcpy(_input_cache2.data(), inputData->data() + halfSize, halfSize);

        LightGrad();
        FindGrad();

        if (output) {
            *(RECT*)output = _cacheresult;
        }
        return 0;
    }

public:
    MainPartFinder() : sizex(0), sizey(0), _lightgrad(10), _threshold(100) {
        _input_cache1.reserve(1980 * 1080 * 4);
        _input_cache2.reserve(1980 * 1080 * 4);
        _light_cache1.reserve(1980 * 1080);
        _light_cache2.reserve(1980 * 1080);
        _fin_cache.reserve(1980 * 1080);
        _supx_cache.reserve(1980);
        _supy_cache.reserve(1080);
        ZeroMemory(&_cacheresult, sizeof(RECT));
    }

    ~MainPartFinder() {}

    int Command(int command, void* input, void* output) {
        switch (command) {
        case 0:
            return Die();
        case 1:
            return Set(input);
        case 3:
            return Work(input, output);
        default:
            return -831;
        }
    }
};