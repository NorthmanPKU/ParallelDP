#pragma once

#include <iostream>
#include <limits>
#include <stdexcept>

template <typename T>
class Tree {
public:
    virtual void prefix_min() = 0;
    virtual T global_min() = 0;
    virtual size_t find_min_index() = 0;
    virtual void remove(size_t pos) = 0;
    virtual ~Tree() {}
};