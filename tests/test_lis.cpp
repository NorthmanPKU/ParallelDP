#include <iostream>
#include <vector>
#include <string>
#include <random>
// #include "../src/include/problem.h"
#include "problem.h"


void checkTest(const std::string &testName, int expected, int got) {
    if (expected != got) {
        std::cout << testName << " Fail: expected: " << expected << ", got " << got << std::endl;
    } else {
        std::cout << testName << " Pass: result is " << got << std::endl;
    }
}


int refSol(std::vector<int>& nums) {
    if (nums.size() <= 1) return nums.size();
    std::vector<int> dp(nums.size(), 1);
    int result = 0;
    for (int i = 1; i < nums.size(); i++) {
        for (int j = 0; j < i; j++) {
            if (nums[i] > nums[j]) dp[i] = std::max(dp[i], dp[j] + 1);
        }
        if (dp[i] > result) result = dp[i];
    }
    return result;
}

std::vector<int> generateRandomInputData(int n, int lowerBound = 0, int upperBound = 100) {
    std::vector<int> data;
    data.reserve(n);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(lowerBound, upperBound);

    for (int i = 0; i < n; ++i) {
        data.push_back(dis(gen));
    }
    return data;
}

int main() {
    {
        std::vector<int> randomData = generateRandomInputData(1000, 1, 10000);
        int refResult = refSol(randomData);
        LIS<int> lis;
        int result = lis.compute(randomData);
        checkTest("Random", refResult, result);
    }

    // {
    //     std::vector<int> data = {10, 22, 9, 33, 21, 50, 41, 60, 80};
    //     LIS<int> lis;
    //     int length = lis.compute(data);
    //     checkTest("Test 1", 6, length);
    // }

    
    // {
    //     std::vector<int> empty;
    //     LIS<int> lis;
    //     int length = lis.compute(empty);
    //     checkTest("Test 2", 0, length);
    // }

    // {
    //     std::vector<int> descending = {9, 8, 7, 6, 5};
    //     LIS<int> lis;
    //     int length = lis.compute(descending);
    //     checkTest("Test 3", 1, length);
    // }

    // {
    //     std::vector<int> ascending = {1, 2, 3, 4, 5};
    //     LIS<int> lis;
    //     int length = lis.compute(ascending);
    //     checkTest("Test 4", 5, length);
    // }

    // {
    //     std::vector<std::string> words = {"apple", "banana", "apricot", "cherry", "date"};
    //     LIS<std::string> lis;
    //     int length = lis.compute(words);
    //     checkTest("Test 5", 4, length);
    // }

    // {
    //     std::vector<int> ascending = {1, 2, 3, 4, 5};
    //     LIS<int, std::greater<int>> lis;
    //     int length = lis.compute(ascending, std::greater<int>());
    //     checkTest("Test 6", 1, length);
    // }

    std::cout << "Test Finished." << std::endl;
    return 0;
}