#include "mainpartfinder.h"
#include <iostream>

MainPartFinder::MainPartFinder()
    : _downer(nullptr)
    , _downer_lock(nullptr)
    , _uper(nullptr)
    , _uper_lock(nullptr)
    , _light_threshold(100)
    , _num_threshold(100)
    , _gape_time(60)
    , _input(nullptr)
    , _output(nullptr)
    , _iswork(false)
    , _islife(true)
    , _cur_cmd(0)
    , _thiscv(nullptr)
    , _thismtx(nullptr) {
}

MainPartFinder::~MainPartFinder() {
    if (_islife) {
        OnUnload();
    }
}

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

    // 等待_gape_time ms
    std::this_thread::sleep_for(std::chrono::milliseconds(_gape_time));

    // lock(_uper_lock) wait(_uper的nos) 拷贝到_cur
    if (_uper && _uper_lock) {
        std::unique_lock<std::mutex> lock(*_uper_lock);
        _uper->wait(lock, [this]() { return !_input->empty(); });
        _cur = _input->clone();
    }

    return 0;
}

int MainPartFinder::Framecut() {
    if (_pre.empty() || _cur.empty()) {
        return 12201;
    }

    // 两帧直接做帧差（已经是灰度图）
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
        // 直接操作_output，Save时会保存
        return 12203;
    }

    // 结果暂存到成员变量，Save时输出
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

int MainPartFinder::Save() {
    if (!_output) {
        return 12102;
    }

    // 如果_output已被GetMain填充，直接返回成功
    // 如果GetMain返回全图，这里需要设置全图范围
    if (_output->left == 0 && _output->top == 0 &&
        _output->right == 0 && _output->bottom == 0) {
        // 如果light为空，无法获取尺寸
        if (_light.empty()) {
            return 12203;
        }
        _output->left = 0;
        _output->top = 0;
        _output->right = _light.cols - 1;
        _output->bottom = _light.rows - 1;
    }

    return 0;
}

void MainPartFinder::Work() {
    while (_islife) {
        // wait(_thiscv) 等待自身广播
        if (_thiscv && _thismtx) {
            std::unique_lock<std::mutex> lock(*_thismtx);
            _thiscv->wait(lock, [this]() { return _cur_cmd != 0 || !_islife; });
        }

        if (!_islife) break;

        if (_iswork) {
            switch (_cur_cmd) {
            case 1: {
                int ret = GetFrame();
                if (ret != 0) {
                    std::cout << "[MainPartFinder] GetFrame失败: " << ret << std::endl;
                    break;
                }
                ret = Framecut();
                if (ret != 0) {
                    std::cout << "[MainPartFinder] Framecut失败: " << ret << std::endl;
                    break;
                }
                ret = Threshold();
                if (ret != 0) {
                    std::cout << "[MainPartFinder] Threshold失败: " << ret << std::endl;
                    break;
                }
                ret = GetMain();
                if (ret != 0) {
                    std::cout << "[MainPartFinder] GetMain失败: " << ret << std::endl;
                    // 不break，继续Save保存全图
                }
                ret = Save();
                if (ret != 0) {
                    std::cout << "[MainPartFinder] Save失败: " << ret << std::endl;
                    break;
                }

                // 如果存在downer，通知下一级
                if (_downer && _downer_lock) {
                    std::lock_guard<std::mutex> lock(*_downer_lock);
                    _downer->notify_one();
                }
                break;
            }
            case 0:
                // 等待50ms防止自身锁失效
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                break;
            default:
                break;
            }
        }

        _cur_cmd = 0;  // 复位
    }
}

int MainPartFinder::OnInit() {
    // 检查环境是否完全
    if (!_input) {
        return 12101;
    }
    if (!_output) {
        return 12102;
    }
    if (!_uper || !_uper_lock) {
        return 12103;
    }

    // 创建自身响应锁
    _thiscv = new std::condition_variable();
    _thismtx = new std::mutex();

    _iswork = true;
    _islife = true;

    // 启动工作线程
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

int MainPartFinder::ICache_config(cache_config config) {
    if (config.input) {
        _input = reinterpret_cast<cv::Mat*>(config.input);
    }
    if (config.output) {
        _output = reinterpret_cast<RECT*>(config.output);
    }
    return 0;
}


int MainPartFinder::IWroking_cofig(int cmd, std::any config) {
    switch (cmd) {
    case 1: {  // 配置明度分割阈值 (light)
        auto* p = std::any_cast<int>(&config);
        if (p && *p > 0 && *p < 255) {
            _light_threshold = *p;
        } else {
            return 11010;  // 参数越界
        }
        return 0;
    }
    case 2: {  // 配置边界筛选阈值 (num)
        auto* p = std::any_cast<int>(&config);
        if (p && *p > 0) {
            _num_threshold = *p;
        } else {
            return 11010;  // 参数越界
        }
        return 0;
    }
    case 3: {  // 配置两帧间隔时间 (gape)
        auto* p = std::any_cast<int>(&config);
        if (p && *p > 0) {
            _gape_time = *p;
        } else {
            return 11010;  // 参数越界
        }
        return 0;
    }
    default:
        return 11001;  // 未知命令
    }
}

int MainPartFinder::IWorking_cmd(int cmd, void* input, void* output) {
    switch (cmd) {
    case 1: {
        // 如果传入了output，link到_output
        if (output) {
            _output = reinterpret_cast<RECT*>(output);
        }
        _cur_cmd = 1;
        // 广播自身响应锁
        if (_thiscv && _thismtx) {
            _thiscv->notify_one();
        }
        return 0;
    }
    default:
        return 11001;
    }
}