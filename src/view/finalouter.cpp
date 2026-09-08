#include "finalouter.h"
#include <iostream>

FinalOutput::FinalOutput()
    : _downer(nullptr)
    , _downer_lock(nullptr)
    , _uper(nullptr)
    , _uper_lock(nullptr)
    , _offset{0, 0, 0, 0}
    , _input(nullptr)
    , _output(nullptr)
    , _iswork(false)
    , _islife(true) {
}

FinalOutput::~FinalOutput() {
    if (_islife) {
        OnUnload();
    }
}

int FinalOutput::SaveFrame() {
    if (!_input) {
        return 13101;
    }

    if (_input->empty()) {
        return 13201;
    }

    _cache = _input->clone();
    return 0;
}

int FinalOutput::Cut() {
    if (_cache.empty()) {
        return 13202;
    }

    int h = _cache.rows;
    int w = _cache.cols;

    // 如果RECT={0,0,0,0}直接拷贝
    if (_offset.left == 0 && _offset.top == 0 &&
        _offset.right == 0 && _offset.bottom == 0) {
        _cut_cache = _cache.clone();
        return 0;
    }

    // 检查裁剪区域是否出界
    if (_offset.left < 0 || _offset.top < 0 ||
        _offset.right >= w || _offset.bottom >= h ||
        _offset.left > _offset.right || _offset.top > _offset.bottom) {
        std::cout << "[FinalOutput] ❌ 裁剪区域出界: "
                  << "left=" << _offset.left
                  << ", top=" << _offset.top
                  << ", right=" << _offset.right
                  << ", bottom=" << _offset.bottom
                  << " (图像尺寸: " << w << "x" << h << ")"
                  << std::endl;
        return 13205;
    }

    // 裁剪
    _cut_cache = _cache(cv::Range(_offset.top, _offset.bottom + 1),
                        cv::Range(_offset.left, _offset.right + 1)).clone();

    return 0;
}

int FinalOutput::TurnL() {
    if (_cut_cache.empty()) {
        return 13203;
    }

    // 转化为单通道灰度图
    cv::cvtColor(_cut_cache, _cut_cache, cv::COLOR_BGRA2GRAY);

    return 0;
}

int FinalOutput::Save() {
    if (!_output) {
        return 13102;
    }

    if (_cut_cache.empty()) {
        return 13204;
    }

    *_output = _cut_cache.clone();
    return 0;
}

void FinalOutput::Work() {
    while (_islife) {
        if (_iswork) {
            // wait(_uper) 等待上级广播
            if (_uper && _uper_lock) {
                std::unique_lock<std::mutex> lock(*_uper_lock);
                _uper->wait(lock, [this]() { return !_islife; });
            }

            if (!_islife) break;

            // SaveFrame()
            int ret = SaveFrame();
            if (ret != 0) {
                std::cout << "[FinalOutput] SaveFrame失败: " << ret << std::endl;
                continue;
            }

            // Cut()
            ret = Cut();
            if (ret != 0) {
                std::cout << "[FinalOutput] Cut失败: " << ret << std::endl;
                continue;
            }

            // TurnL()
            ret = TurnL();
            if (ret != 0) {
                std::cout << "[FinalOutput] TurnL失败: " << ret << std::endl;
                continue;
            }

            // 如果存在downer且有效
            if (_downer && _downer_lock) {
                // Save()
                ret = Save();
                if (ret != 0) {
                    std::cout << "[FinalOutput] Save失败: " << ret << std::endl;
                    continue;
                }

                // _downer.notify() 向下广播
                std::lock_guard<std::mutex> lock(*_downer_lock);
                _downer->notify_one();
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

int FinalOutput::OnInit() {
    // 检查环境是否完全
    if (!_input) {
        return 13101;
    }
    if (!_output) {
        return 13102;
    }
    if (!_uper || !_uper_lock) {
        return 13103;
    }

    _iswork = true;
    _islife = true;

    // 启动工作线程
    if (_workThread.joinable()) {
        return 13302;
    }

    try {
        _workThread = std::thread(&FinalOutput::Work, this);
    } catch (...) {
        return 13301;
    }

    std::cout << "[FinalOutput] 初始化成功" << std::endl;
    return 0;
}

int FinalOutput::OnPause() {
    if (!_iswork) {
        return 13301;
    }
    _iswork = false;
    return 0;
}

int FinalOutput::OnResume() {
    if (_iswork) {
        return 13302;
    }
    _iswork = true;
    return 0;
}

int FinalOutput::OnUnload() {
    _islife = false;
    _iswork = false;

    if (_uper && _uper_lock) {
        _uper->notify_all();
    }

    if (_workThread.joinable()) {
        try {
            _workThread.join();
        } catch (...) {
            return 13302;
        }
    }

    _cache.release();
    _cut_cache.release();

    std::cout << "[FinalOutput] 已卸载" << std::endl;
    return 0;
}

int FinalOutput::ILock_config(lock_config config) {
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

int FinalOutput::ICache_config(cache_config config) {
    if (config.input) {
        _input = reinterpret_cast<cv::Mat*>(config.input);
    }
    if (config.output) {
        _output = reinterpret_cast<cv::Mat*>(config.output);
    }
    return 0;
}

/*
 * IWroking_cofig - 工作配置函数（cmd模式）
 *
 * 命令列表：
 *   cmd=1: 配置裁剪区域 (RECT)
 *          config 类型: std::any 存储 RECT
 *          示例: config = rect;  // RECT 类型
 *
 * 返回值：
 *   0  - 成功
 *   11004 - 类型不匹配
 */
int FinalOutput::IWroking_cofig(int cmd, std::any config) {
    switch (cmd) {
    case 1: {  // 配置裁剪区域 (RECT)
        auto* p = std::any_cast<RECT>(&config);
        if (p) {
            _offset = *p;
        } else {
            return 11004;  // 类型不匹配
        }
        return 0;
    }
    default:
        return 11001;  // 未知命令
    }
}

int FinalOutput::IWorking_cmd(int cmd, void* input, void* output) {
    switch (cmd) {
    case 1: {  // 手动触发一次处理（预留）
        if (_uper && _uper_lock) {
            _uper->notify_one();
        }
        return 0;
    }
    default:
        return 11001;
    }
}