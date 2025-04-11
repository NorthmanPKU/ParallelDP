#include <iostream>
#include <vector>
#include <string>
#include "../src/include/problem.h"


void checkTest(const std::string &testName, int expected, int got) {
    if (expected != got) {
        std::cout << testName << " Fail: expected: " << expected << ", got " << got << std::endl;
    } else {
        std::cout << testName << " Pass: result is " << got << std::endl;
    }
}

int main() {
    {
        std::vector<int> data = {10, 22, 9, 33, 21, 50, 41, 60, 80};
        LIS<int> lis;
        int length = lis.compute(data);
        checkTest("Test 1", 6, length);
    }

    {
        std::vector<int> empty;
        LIS<int> lis;
        int length = lis.compute(empty);
        checkTest("Test 2", 0, length);
    }

    {
        std::vector<int> descending = {9, 8, 7, 6, 5};
        LIS<int> lis;
        int length = lis.compute(descending);
        checkTest("Test 3", 1, length);
    }

    {
        std::vector<int> ascending = {1, 2, 3, 4, 5};
        LIS<int> lis;
        int length = lis.compute(ascending);
        checkTest("Test 4", 5, length);
    }

    {
        std::vector<std::string> words = {"apple", "banana", "apricot", "cherry", "date"};
        LIS<std::string> lis;
        int length = lis.compute(words);
        checkTest("Test 5", 4, length);
    }

    {
        std::vector<int> ascending = {1, 2, 3, 4, 5};
        LIS<int, std::greater<int>> lis;
        int length = lis.compute(ascending, std::greater<int>());
        checkTest("Test 6", 1, length);
    }

    std::cout << "Test Finished." << std::endl;
    return 0;
}