#pragma once
//utils.h

#include<iostream>
#include<sstream>

template <typename... Args>
std::string stringCombination(const Args&... args) {

    std::ostringstream oss;

    ((oss << args), ...);

    std::string str = oss.str();

    return str;
}