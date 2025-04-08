#pragma once

#include <vector>
#include <functional>
#include <limits>
#include <string>


// Use the Cordon algorithm to solve the Longest Increasing Subsequence (LIS) problem, 
// supporting any data type T and user-defined comparison functions.
template<typename T, typename Compare = std::less<T>>
class LIS {
public:
    // The parameter cmp is a comparison function, defaulting to std::less<T>
    int compute(const std::vector<T>& data, Compare cmp = Compare());
};


// Use the Cordon algorithm to solve the Longest Common Subsequence (LCS) problem, 
// supporting any data type T and user-defined comparison functions.
template<typename T, typename Compare = std::less<T>>
class LCS {
public:
    // The parameter cmp is a comparison function, defaulting to std::less<T>
    int compute(const std::vector<T>& data1, const std::vector<T>& data2, Compare cmp = Compare());
};