#include <iostream>
#include <vector>
#include <string>
#include "problem.h"


void checkTest(const std::string &testName, int expected, int got) {
    if (expected != got) {
        std::cout << testName << " Fail: expected: " << expected << ", got " << got << std::endl;
    } else {
        std::cout << testName << " Pass: result is " << got << std::endl;
    }
}

int main() {
    // {
    //     std::string s1 = "ABCBDAB";
    //     std::string s2 = "BDCABA";
    //     LCS<char> lcs;
    //     int length = lcs.compute(s1, s2);
    //     checkTest("Test 1", 4, length);
    // }

    // {
    //     std::string s1 = "";
    //     std::string s2 = "BDCABA";
    //     LCS<char> lcs;
    //     int length = lcs.compute(s1, s2);
    //     checkTest("Test 2", 0, length);
    // }

    // {
    //     std::string s1 = "";
    //     std::string s2 = "";
    //     LCS<char> lcs;
    //     int length = lcs.compute(s1, s2);
    //     checkTest("Test 3", 0, length);
    // }

    // {
    //     std::string s1 = "AGGTAB";
    //     std::string s2 = "AGGTAB";
    //     LCS<char> lcs;
    //     int length = lcs.compute(s1, s2);
    //     checkTest("Test 4", static_cast<int>(s1.size()), length);
    // }

    {
        std::vector<int> v1 = {1, 3, 4, 1, 2, 3};
        std::vector<int> v2 = {3, 4, 1, 2, 1, 3};
        LCS<int> lcs;
        int length = lcs.compute(v1, v2);
        checkTest("Test 5", 5, length);
    }

    std::cout << "Test Finished." << std::endl;
    return 0;
}