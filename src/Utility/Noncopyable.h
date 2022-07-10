//
// Created by x on 22-7-7.
//

#ifndef XLTOOLKIT_NONCOPYABLE_H
#define XLTOOLKIT_NONCOPYABLE_H

namespace utility
{

class Noncopyable {
public:
    Noncopyable(const Noncopyable&) = delete;
    Noncopyable operator=(const Noncopyable&) = delete;
protected:
    Noncopyable() = default;
    ~Noncopyable() = default;
};

}

#endif //XLTOOLKIT_NONCOPYABLE_H
