#include <iostream>
#include <vector>
#include <string>
#include "lcs.h"
#include "utils.h"
#include <chrono>

void checkTest(const std::string &testName, int expected, int got) {
    if (expected != got) {
        std::cout << testName << " Fail: expected: " << expected << ", got " << got << std::endl;
    } else {
        std::cout << testName << " Pass: result is " << got << std::endl;
    }
}

int main() {
    bool parallel = true;
    // {
    //     std::string s1 = "ABCBDAB";
    //     std::string s2 = "BDCABA";
    //     LCS<char> lcs;
    //     int length = lcs.compute(s1, s2, parallel);
    //     checkTest("Test 1", 4, length);
    // }

    // {
    //     std::string s1 = "";
    //     std::string s2 = "BDCABA";
    //     LCS<char> lcs;
    //     int length = lcs.compute(s1, s2, parallel);
    //     checkTest("Test 2", 0, length);
    // }

    // {
    //     std::string s1 = "";
    //     std::string s2 = "";
    //     LCS<char> lcs;
    //     int length = lcs.compute(s1, s2, parallel);
    //     checkTest("Test 3", 0, length);
    // }

    // {
    //     std::string s1 = "AGGTAB";
    //     std::string s2 = "AGGTAB";
    //     LCS<char> lcs;
    //     int length = lcs.compute(s1, s2, parallel);
    //     checkTest("Test 4", static_cast<int>(s1.size()), length);
    // }

    {
        // std::vector<int> v1 = {1, 3, 4, 1, 2, 3};
        // std::vector<int> v2 = {3, 4, 1, 2, 1, 3};
        std::vector<int> v1, v2;
        std::tie(v1, v2) = generateLCS(10000000, 10000000, 10);
        LCS<int> lcs;
        // time the execution of the function
        auto start = std::chrono::high_resolution_clock::now();
        int length = lcs.compute(v1, v2, parallel, 2500);
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "Time taken in parallel: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
        checkTest("Parallel: ", 10, length);

        auto start2 = std::chrono::high_resolution_clock::now();
        int length2 = lcs.compute(v1, v2, false);
        auto end2 = std::chrono::high_resolution_clock::now();
        std::cout << "Time taken in serial: " << std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2).count() << "ms" << std::endl;
        checkTest("Serial: ", 10, length2);

        auto start3 = std::chrono::high_resolution_clock::now();
        int length3 = lcs_dp_naive(v1, v2);
        auto end3 = std::chrono::high_resolution_clock::now();
        std::cout << "Time taken in naive: " << std::chrono::duration_cast<std::chrono::milliseconds>(end3 - start3).count() << "ms" << std::endl;

        checkTest("Naive: ", 10, length3);
    }

    std::cout << "Test Finished." << std::endl;
    return 0;
}