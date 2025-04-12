// src/main.cpp
#include <iostream>
#include <vector>
#include <string>
#include "problem.h"

int main(int argc, char* argv[]) {
    std::vector<int> seq1 = {1, 2, 3, 4, 5};
    std::vector<int> seq2 = {3, 1, 4, 2, 5};
    
    LCS<int> lcs;
    int result = lcs.compute(seq1, seq2);
    
    std::cout << "LCS length: " << result << std::endl;
    return 0;
}