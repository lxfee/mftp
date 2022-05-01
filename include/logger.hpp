#pragma once
#include <string>
#include <iostream>
inline void logger(std::string msg) {
    std::cout << msg << std::endl;
}


// 使用模板，不能分离编译，因为有个模板特化过程，特化完后才是真正的函数。
// 为了防止符号重复定义，使用inline
template <class T>
inline void logger(std::string msg, T id) {
    std::cout << "[" << " " << id << " " << "] ";
    std::cout << msg << std::endl;
}
