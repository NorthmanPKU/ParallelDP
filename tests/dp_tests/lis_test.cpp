// src/main.cpp
#include <iostream>
#include <vector>
#include <string>
#include "lis.h"

int main(int argc, char* argv[]) {
    // std::vector<int> seq1 = {1, 2, 3, 4, 5};
    std::vector<int> seq2 = {3, 1, 4, 2, 5};
    
    LIS<int> lis;
    int result = lis.compute(seq2);
    
    std::cout << "LIS length: " << result << std::endl;
    return 0;
}