//
// Created by x on 22-7-11.
//

#include <sys/ioctl.h>
#include "PipeWrapper.h"
#include <iostream>
#include <stdexcept>

PipeWrapper::~PipeWrapper() {
    clearFD();
}

PipeWrapper::PipeWrapper() {
    if (pipe(pipe_fd_) == -1) {
        throw std::runtime_error("pipe ctrate error");
    }
    ioctl(pipe_fd_[0], FIONBIO, true);
    ioctl(pipe_fd_[1], FIONBIO, false);
}

inline int PipeWrapper::getReadFD() const {
    return pipe_fd_[0];
}

inline int PipeWrapper::getWriteFD() const{
    return pipe_fd_[1];
}

void PipeWrapper::clearFD() {
    if (pipe_fd_[0] != -1) {
        close(pipe_fd_[0]);
        pipe_fd_[0] = -1;
    }
    if (pipe_fd_[1] != -1) {
        close(pipe_fd_[1]);
        pipe_fd_[1] = -1;
    }
}

size_t PipeWrapper::write(const void *buf, size_t n_bytes) {
    return ::write(pipe_fd_[1], buf, n_bytes);
}

size_t PipeWrapper::read(void *buf, size_t n_bytes) {
    return ::read(pipe_fd_[0], buf, n_bytes);
}
