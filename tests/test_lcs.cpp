#include <iostream>
#include <vector>
#include <string>
#include "lcs.h"
#include "utils.h"
#include <chrono>
#include "data.h"
#include <cstring>
#include <cstdlib>

void checkTest(const std::string &testName, int expected, int got) {
    if (expected != got) {
        std::cout << testName << " Fail: expected: " << expected << ", got " << got << std::endl;
    } else {
        std::cout << testName << " Pass: result is " << got << std::endl;
    }
}

void printUsage() {
    std::cout << "Usage: ./lcs -n <size1> -m <size2> -k <lcs_length> [-g <granularity>]" << std::endl;
    std::cout << "  -n: size of first dimension (default: 100000)" << std::endl;
    std::cout << "  -m: size of second dimension (default: 100000)" << std::endl;
    std::cout << "  -k: expected LCS length (default: 10)" << std::endl;
    std::cout << "  -g: granularity for parallel processing (default: 5000)" << std::endl;
}

int main(int argc, char* argv[]) {
    bool parallel = true;

    int n = 1000000;
    int m = 1000000;
    int k = 10;
    int granularity = 5000;

    bool test_random = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            n = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            m = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) {
            k = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-g") == 0 && i + 1 < argc) {
            granularity = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-r") == 0) {
            test_random = true;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printUsage();
            return 0;
        } else {
            std::cout << "Invalid argument: " << argv[i] << std::endl;
            printUsage();
            return 0;
        }
    }
    {
        std::vector<std::vector<int>> arrows;
        std::cout << "Generating LCS..." << std::endl;
        if (test_random) {
            arrows = MakeRandom(n, m);
        } else {
            arrows = MakeData(n, m, k);
        }
        // generateLCS(l1, l2, lcs_length, arrows);
        std::cout << "LCS generated." << std::endl;
        LCS<int> lcs;
        // time the execution of the function
        auto start = std::chrono::high_resolution_clock::now();
        // int length = lcs.compute(v1, v2, arrows, parallel, granularity);
        int length = lcs.compute_arrows(arrows, parallel, granularity);
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "Time taken in parallel: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
        checkTest("Parallel: ", k, length);

        auto start2 = std::chrono::high_resolution_clock::now();
        // int length2 = lcs.compute(v1, v2, arrows, false);
        int length2 = lcs.compute_arrows(arrows, false, granularity);
        auto end2 = std::chrono::high_resolution_clock::now();
        std::cout << "Time taken in serial: " << std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2).count() << "ms" << std::endl;
        checkTest("Serial: ", k, length2);
        arrows.clear();

        // auto start3 = std::chrono::high_resolution_clock::now();
        // int length3 = lcs_dp_naive(v1, v2);
        // auto end3 = std::chrono::high_resolution_clock::now();
        // std::cout << "Time taken in naive: " << std::chrono::duration_cast<std::chrono::milliseconds>(end3 - start3).count() << "ms" << std::endl;

        // checkTest("Naive: ", 10, length3);
    }

    std::cout << "Test Finished." << std::endl;
    return 0;
}