#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <cstring>
#include <cstdlib>

#include "lcs.h"
#include "utils.h"
#include "data.h"
#include "parlay/sequence.h"

void checkTest(const std::string &testName, int expected, int got) {
    if (expected != got) {
        std::cout << testName << " Fail: expected: " << expected << ", got " << got << std::endl;
    } else {
        std::cout << testName << " Pass: result is " << got << std::endl;
    }
}

void testLCS(std::vector<std::vector<int>> &arrows, ParallelArch parallelArch, bool parallel, int granularity, int k) {
    std::cout << "--------------------------------" << std::endl;
    LCS<int> lcs;
    auto start = std::chrono::high_resolution_clock::now();
    int length1 = lcs.compute_arrows(arrows, parallelArch, parallel, granularity);
    auto end = std::chrono::high_resolution_clock::now();

    std::string para = parallel ? "parallel" : "sequential";

    switch (parallelArch) {
        case ParallelArch::OPENMP:
            // std::cout << "OpenMP " << para << ": " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
            checkTest("OpenMP " + para + ": ", k, length1 - 1);
            break;
        case ParallelArch::CILK:
            // std::cout << "Cilk " << para << ": " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
            checkTest("Cilk " + para + ": ", k, length1 - 1);
            break;
        default:
            std::cout << "Invalid parallel architecture" << std::endl;
            break;
    }
    std::cout << "--------------------------------" << std::endl;
}

void testLCS_parlay(size_t n, const parlay::sequence<parlay::sequence<size_t>>& arrows, ParallelArch parallelArch, bool parallel, int granularity, int k) {
    std::cout << "--------------------------------" << std::endl;

    LCS<int> lcs;
    auto start = std::chrono::high_resolution_clock::now();
    int length1;
    
    
    switch (parallelArch) {
        case ParallelArch::PARLAY: {
            length1 = lcs.compute_arrows_paralay(n, arrows, parallel, granularity);
            auto end1 = std::chrono::high_resolution_clock::now();
            std::cout << "Parlay: " << std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start).count() << "ms" << std::endl;
            checkTest("Parlay: ", k, length1);
            break;
        }
        case ParallelArch::CILK_OPT: {
            length1 = lcs.compute_arrows_opt(arrows, parallel, granularity);
            auto end2 = std::chrono::high_resolution_clock::now();
            std::cout << "Cilk_opt: " << std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start).count() << "ms" << std::endl;
            checkTest("Cilk_opt: ", k, length1);
            break;
        }
        default: {
            std::cout << "Invalid parallel architecture" << std::endl;
            break;
        }
    }
    std::cout << "--------------------------------" << std::endl;
}

void printUsage() {
    std::cout << "Usage: ./lcs -n <size1> -m <size2> -k <lcs_length> [-g <granularity>]" << std::endl;
    std::cout << "  -n: size of first dimension (default: 100000)" << std::endl;
    std::cout << "  -m: size of second dimension (default: 100000)" << std::endl;
    std::cout << "  -k: expected LCS length (default: 10)" << std::endl;
    std::cout << "  -g: granularity for parallel processing (default: 5000)" << std::endl;
}

int main(int argc, char* argv[]) {

    int n = 1000000;
    int m = 1000000;
    int k = 10;
    int granularity = 5000;
    ParallelArch parallelArch = ParallelArch::PARLAY;
    bool test_random = false;
    bool parallel = true;

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
        } else if (strcmp(argv[i], "-seq") == 0) {
            parallel = false;
        } else if (strcmp(argv[i], "-run") == 0 && i + 1 < argc) {
            // "cilk", "openmp", "parlay"
            if (strcmp(argv[i + 1], "cilk") == 0) {
                parallelArch = ParallelArch::CILK;
            } else if (strcmp(argv[i + 1], "openmp") == 0) {
                parallelArch = ParallelArch::OPENMP;
            } else if (strcmp(argv[i + 1], "parlay") == 0) {
                parallelArch = ParallelArch::PARLAY;
            } else if (strcmp(argv[i + 1], "opt") == 0) {
                parallelArch = ParallelArch::CILK_OPT;
            } else {
                std::cout << "Invalid parallel architecture: " << argv[i + 1] << std::endl;
                printUsage();
                return 0;
            }
            i++;
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printUsage();
            return 0;
        } else {
            std::cout << "Invalid argument: " << argv[i] << std::endl;
            printUsage();
            return 0;
        }
    }

    switch (parallelArch) {
        case ParallelArch::CILK:
        case ParallelArch::OPENMP: {
            std::vector<std::vector<int>> arrows;
            if (test_random) {
                std::cout << "Generating random LCS..." << std::endl;
                arrows = MakeRandom(n, m);
            } else {
                std::cout << "Generating LCS..." << std::endl;
                arrows = MakeData(n, m, k);
            }
            testLCS(arrows, parallelArch, parallel, granularity, k);
            break;
        }
        case ParallelArch::CILK_OPT: 
        case ParallelArch::PARLAY: {
            auto arrows2 = MakeParlayData(n, m, k);
            testLCS_parlay(n, arrows2, parallelArch, parallel, granularity, k);
            // testLCS_parlay(n, arrows2, false, granularity, k);
            break;
        }
    }

    std::cout << "Test Finished." << std::endl;
    return 0;
}