//
// Created by x on 22-7-6.
//

#ifndef XLTOOLKIT_UTIL_H
#define XLTOOLKIT_UTIL_H

//禁止拷贝基类
class noncopyable {
protected:
    noncopyable() {}
    ~noncopyable() {}
private:
    //禁止拷贝
    noncopyable(const noncopyable &that) = delete;
    noncopyable(noncopyable &&that) = delete;
    noncopyable &operator=(const noncopyable &that) = delete;
    noncopyable &operator=(noncopyable &&that) = delete;
};

class Util {

};


#endif //XLTOOLKIT_UTIL_H
