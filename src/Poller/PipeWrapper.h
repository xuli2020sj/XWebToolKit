//
// Created by x on 22-7-11.
//

#ifndef XLTOOLKIT_PIPEWRAPPER_H
#define XLTOOLKIT_PIPEWRAPPER_H

#include <unistd.h>

class PipeWrapper final {
private:
    int pipe_fd_[2] = {-1, -1};
public:
    PipeWrapper();
    ~PipeWrapper();

    inline int getReadFD () const;
    inline int getWriteFD () const;
    void clearFD();

    size_t write(const void *buf, size_t n_bytes);
    size_t read(void *buf, size_t n_bytes);
};


#endif //XLTOOLKIT_PIPEWRAPPER_H
