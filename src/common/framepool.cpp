#include "FramePool.h"

FramePool::FramePool(int h, int w, int t, int framesnum)
    : _h(h), _w(w), _t(t), _framesnum(framesnum), _framecurser(0) {
    _frames_pool.reserve(_framesnum);
    for (int i = 0; i < _framesnum; ++i) {
        _frames_pool.emplace_back();
        _frames_pool.back().frame.create(_h, _w, _t);
    }
}

FramePool::State FramePool::GetState(Frame* frame) {
    if (frame->w_state == 0) {
        return State::Empty;
    }
    else if (frame->w_state == 1) {
        return State::Writing;
    }
    else if (frame->w_state == 2) {
        if (frame->r_count == 0) {
            return State::Readable;
        }
        else {
            return State::Reading;
        }
    }
    return State::Empty;
}

bool FramePool::StateMatch(State state, Command cmd) {
    if (cmd == Command::Write) {
        return (state == State::Empty || state == State::Readable);
    }
    else if (cmd == Command::Read) {
        return (state == State::Readable || state == State::Reading);
    }
    return false;
}

Frame* FramePool::MovePtr(Command cmd) {
    std::lock_guard<std::mutex> lock(_cursorMutex);

    for (int i = 0; i < _framesnum; ++i) {
        int index = (_framecurser + i) % _framesnum;
        Frame* frame = &_frames_pool[index];
        State state = GetState(frame);

        if (StateMatch(state, cmd)) {
            _framecurser = (index + 1) % _framesnum;
            return frame;
        }
    }
    return nullptr;
}

int FramePool::GetFrame(Frame*& curframe) {
    Frame* frame = MovePtr(Command::Read);
    if (frame == nullptr) {
        return -1;
    }
    frame->r_count++;
    curframe = frame;
    return 0;
}

int FramePool::SetFrame(Frame*& curframe) {
    Frame* frame = MovePtr(Command::Write);
    if (frame == nullptr) {
        return -1;
    }
    frame->w_state = 1;        //在写
    curframe = frame;
    return 0;
}

void FramePool::FinishWrite(Frame* frame) {
    if (frame) {
        frame->w_state = 2;     //可读
        frame->r_count = 0;
    }
}

void FramePool::ReturnFrame(Frame* frame) {
    if (frame && frame->r_count > 0) {
        frame->r_count--;
    }
}