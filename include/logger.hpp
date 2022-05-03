#pragma once
#include <string>
#include <iostream>
#include <ctime>

extern bool debugflag;


template <class T>
inline void logger(T msg) {
    if(debugflag) {
        std::cout << time(0) << "\t";
        std::cout << msg << std::endl;
    }
}

// 使用模板，不能分离编译，因为有个模板特化过程，特化完后才是真正的函数。
// 为了防止符号重复定义，使用inline
template <class T1, class T2>
inline void logger(T1 msg, T2 id) {
    if(debugflag) {
        std::cout << time(0) << "\t";
        std::cout << "[" << " " << id << " " << "] " << "\t";
        std::cout << msg << std::endl;
    }
}
