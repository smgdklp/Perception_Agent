#include "mainpartfinder.h"
#include <iostream>

MainPartFinder::MainPartFinder()
    : _downer(nullptr)
    , _downer_lock(nullptr)
    , _uper(nullptr)
    , _uper_lock(nullptr)
    , _light_threshold(100)
    , _num_threshold(50)
    , _input(nullptr)
    , _output(nullptr)
    , _iswork(false)
    , _islife(true)
    , _cur_cmd(0)
    , _thiscv(nullptr)
    , _thismtx(nullptr) {
    _persize = {0, 0, 0, 0};
}

MainPartFinder::~MainPartFinder() {
    if (_islife) {
        OnUnload();
    }
}

// 拷贝
int MainPartFinder::GetFrame() {
    if (!_input) {
        return 12101;
    }

    // lock(_uper_lock) wait(_uper的nos) 直接从_input拷贝到_pre
    if (_uper && _uper_lock) {
        std::unique_lock<std::mutex> lock(*_uper_lock);
        _uper->wait(lock, [this]() { return !_input->empty(); });
        _pre = _input->clone();
    }

    // 等待60ms
    std::this_thread::sleep_for(std::chrono::milliseconds(60));

    // lock(_uper_lock) wait(_uper的nos) 拷贝到_cur
    if (_uper && _uper_lock) {
        std::unique_lock<std::mutex> lock(*_uper_lock);
        _uper->wait(lock, [this]() { return !_input->empty(); });
        _cur = _input->clone();
    }

    return 0;
}

// 转化为二值图
int MainPartFinder::TurnL() {
    if (_pre.empty() || _cur.empty()) {
        return 12202;
    }

    // 两帧直接在原对象转化为单通道灰度图
    cv::cvtColor(_pre, _pre, cv::COLOR_BGRA2GRAY);
    cv::cvtColor(_cur, _cur, cv::COLOR_BGRA2GRAY);

    return 0;
}

int MainPartFinder::Framecut() {
    if (_pre.empty() || _cur.empty()) {
        return 12201;
    }

    // 做帧差
    cv::absdiff(_pre, _cur, _light);

    return 0;
}

int MainPartFinder::Threshold() {
    if (_light.empty()) {
        return 12202;
    }

    // 硬二分：大于_light_threshold的设为0，小于等于_light_threshold的设为1
    cv::threshold(_light, _light, _light_threshold, 1, cv::THRESH_BINARY_INV);

    return 0;
}

int MainPartFinder::GetMain() {
    if (_light.empty()) {
        return 12203;
    }

    int h = _light.rows;
    int w = _light.cols;

    // x轴求和（每一列的白点数量，白点=1）
    std::vector<int> x_count(w, 0);
    for (int y = 0; y < h; ++y) {
        const uchar* row = _light.ptr<uchar>(y);
        for (int x = 0; x < w; ++x) {
            if (row[x] == 1) {
                x_count[x]++;
            }
        }
    }

    // y轴求和（每一行的白点数量，白点=1）
    std::vector<int> y_count(h, 0);
    for (int y = 0; y < h; ++y) {
        const uchar* row = _light.ptr<uchar>(y);
        for (int x = 0; x < w; ++x) {
            if (row[x] == 1) {
                y_count[y]++;
            }
        }
    }

    int threshold = _num_threshold;

    // 从高位遍历找第一个大于threshold的index (x_max)
    int x_max = -1;
    for (int i = w - 1; i >= 0; --i) {
        if (x_count[i] > threshold) {
            x_max = i;
            break;
        }
    }

    // 从低位遍历找第一个大于threshold的index (x_min)
    int x_min = -1;
    for (int i = 0; i < w; ++i) {
        if (x_count[i] > threshold) {
            x_min = i;
            break;
        }
    }

    // 从高位遍历找第一个大于threshold的index (y_max)
    int y_max = -1;
    for (int i = h - 1; i >= 0; --i) {
        if (y_count[i] > threshold) {
            y_max = i;
            break;
        }
    }

    // 从低位遍历找第一个大于threshold的index (y_min)
    int y_min = -1;
    for (int i = 0; i < h; ++i) {
        if (y_count[i] > threshold) {
            y_min = i;
            break;
        }
    }

    // 如果没找到有效边界，返回全图范围
    if (x_min == -1 || x_max == -1 || y_min == -1 || y_max == -1) {
        std::cout << "[MainPartFinder] ⚠️ 未找到有效主成分 (threshold=" 
                  << threshold << ")，返回全图" << std::endl;
        _output->left = 0;
        _output->top = 0;
        _output->right = w - 1;
        _output->bottom = h - 1;
        return 0;
    }

    // 结果转化为RECT返回到_output
    _output->left = x_min;
    _output->top = y_min;
    _output->right = x_max;
    _output->bottom = y_max;

    // 最终打印rect在控制台
    std::cout << "[MainPartFinder] 结果 RECT: left=" << _output->left
              << ", top=" << _output->top
              << ", right=" << _output->right
              << ", bottom=" << _output->bottom
              << " (宽=" << (_output->right - _output->left)
              << ", 高=" << (_output->bottom - _output->top) << ")"
              << std::endl;

    return 0;
}

// 主线循环
void MainPartFinder::Work() {
    while (_islife) {
        // wait(_thismtx) 等待自身函数调用
        if (_thiscv && _thismtx) {
            std::unique_lock<std::mutex> lock(*_thismtx);
            _thiscv->wait(lock, [this]() { return _cur_cmd != 0 || !_islife; });
        }

        if (!_islife) break;

        if (_iswork) {
            switch (_cur_cmd) {
            case 1:
                GetFrame();
                TurnL();
                Framecut();
                Threshold();
                GetMain();

                // 如果存在downer，就通知下一级
                if (_downer && _downer_lock) {
                    std::lock_guard<std::mutex> lock(*_downer_lock);
                    _downer->notify_one();
                }
                break;
            case 0:
                // sleep(60) 细节睡一下再pass防止锁失效
                std::this_thread::sleep_for(std::chrono::milliseconds(60));
                break;
            default:
                break;
            }
        }

        _cur_cmd = 0;  // 细节复位函数
    }
}

// 必备生命周期函数
int MainPartFinder::OnInit() {
    // 检查环境是否完全，只检验上层指针是否有效
    if (!_input) {
        return 12101;
    }
    if (!_output) {
        return 12102;
    }

    // new condition_variable* _thiscv; mutex* _thismtx; 作为自身被动响应的锁对象
    _thiscv = new std::condition_variable();
    _thismtx = new std::mutex();

    _iswork = true;
    _islife = true;

    // 创立线程Work运行
    if (_workThread.joinable()) {
        return 12302;
    }

    try {
        _workThread = std::thread(&MainPartFinder::Work, this);
    } catch (...) {
        return 12301;
    }

    std::cout << "[MainPartFinder] 初始化成功" << std::endl;
    return 0;
}

int MainPartFinder::OnPause() {
    if (!_iswork) {
        return 12301;
    }
    _iswork = false;
    return 0;
}

int MainPartFinder::OnResume() {
    if (_iswork) {
        return 12302;
    }
    _iswork = true;
    return 0;
}

int MainPartFinder::OnUnload() {
    _islife = false;
    _iswork = false;

    if (_thiscv) {
        _thiscv->notify_all();
    }

    if (_workThread.joinable()) {
        try {
            _workThread.join();
        } catch (...) {
            return 12302;
        }
    }

    if (_thiscv) {
        delete _thiscv;
        _thiscv = nullptr;
    }
    if (_thismtx) {
        delete _thismtx;
        _thismtx = nullptr;
    }

    _pre.release();
    _cur.release();
    _light.release();

    std::cout << "[MainPartFinder] 已卸载" << std::endl;
    return 0;
}

// 缓存到condition_variable* _downer; mutex* _downer_lock; condition_variable* _uper; mutex* _uper_lock;
int MainPartFinder::ILock_config(lock_config config) {
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

// 缓存指针索引到_output和_input
int MainPartFinder::ICache_config(cache_config config) {
    if (config.input) {
        _input = reinterpret_cast<cv::Mat*>(config.input);
    }
    if (config.output) {
        _output = reinterpret_cast<RECT*>(config.output);
    }
    return 0;
}

// 设计缺陷问题，只能采用单字典对来赋值
int MainPartFinder::IWroking_cofig(working_config config) {
    if (config.type == ConfigType::INT) {
        try {
            auto kv = std::any_cast<std::pair<std::string, int>>(config.info);
            if (kv.first == "num") {
                _num_threshold = kv.second;
            } else if (kv.first == "light") {
                _light_threshold = kv.second;
            } else {
                return 11004;
            }
        } catch (const std::bad_any_cast&) {
            return 11004;
        }
    } else {
        return 11004;
    }
    return 0;
}

// cmd:1，执行一次主区域提取
int MainPartFinder::IWorking_cmd(int cmd, void* input, void* output) {
    switch (cmd) {
    case 1:
        _cur_cmd = 1;
        // 释放自身响应锁
        if (_thiscv && _thismtx) {
            _thiscv->notify_one();
        }
        return 0;
    default:
        return 11001;
    }
}