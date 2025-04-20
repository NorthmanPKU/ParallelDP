#include <iostream>
#include <vector>
#include <algorithm>
#include <climits>
#include "glws.h"


void checkTest(const std::string &testName, long long expected, long long got) {
    if (expected != got) {
        std::cout << testName << " Fail: expected: " << expected << ", got " << got << std::endl;
    } else {
        std::cout << testName << " Pass: result is " << got << std::endl;
    }
}


long double computeCost(const std::vector<long double>& x, const std::vector<long double>& prefixSum, size_t i, size_t j) {
    size_t l = i + 1, r = j;
    if (l > r) return 0;

    size_t mid = (l + r) / 2;
    long double median = x[mid - 1];
    long double left = median * (mid - l + 1) - (prefixSum[mid] - prefixSum[l - 1]);
    long double right = (prefixSum[r] - prefixSum[mid]) - median * (r - mid);
    return left + right;
}

long double refSol(const std::vector<long double>& input, long double buildCost) {
    size_t n = input.size();
    if (n == 0) return 0;

    std::vector<long double> x = input;
    std::sort(x.begin(), x.end());

    std::vector<long double> prefixSum(n + 1, 0);
    for (size_t i = 1; i <= n; ++i)
        prefixSum[i] = prefixSum[i - 1] + x[i - 1];

    std::vector<long double> E(n + 1, 0);
    std::vector<size_t> backtrack(n + 1, 0);

    for (size_t j = 1; j <= n; ++j) {
        E[j] = std::numeric_limits<long double>::max();
        for (size_t i = 0; i < j; ++i) {
            long double cost = computeCost(x, prefixSum, i, j) + buildCost;
            if (E[i] + cost < E[j]) {
                E[j] = E[i] + cost;
                backtrack[j] = i;
            }
        }
    }

    std::vector<std::pair<size_t, size_t>> segments;
    for (size_t t = n; t > 0; t = backtrack[t]) {
        segments.emplace_back(backtrack[t] + 1, t);
    }
    std::reverse(segments.begin(), segments.end());

    std::cout << "Total cost: " << E[n] << '\n';
    std::cout << "Used " << segments.size() << " segments\n";
    for (const auto& [l, r] : segments) {
        std::cout << "  Segment [" << l << ", " << r << "]: ";
        for (size_t i = l; i <= r; ++i) std::cout << x[i - 1] << " ";
        std::cout << '\n';
    }

    return E[n];
}

int main() {
    std::vector<long double> pos = {1, 2, 3, 7, 8, 9, 10};
    int buildCost = 10;

    auto costFunc = [&](int j, int i, const std::vector<long double>& pos) -> long double {  // [j+1, i]
        if (i - j < 1) return buildCost;
        int len = i - j;
        int mid_idx = j + 1 + (len - 1) / 2;
        long double median = pos[mid_idx];
        long double cost = 0;
        for (int k = j + 1; k <= i; ++k) {
          cost += std::abs(pos[k] - median);
        }
        return cost + buildCost;
    };

    // auto efunc = [](long double d, int j) { return d; };

    ConvexGLWS<long double> glws;
    long double result = glws.compute(pos, costFunc);
    checkTest("GLWS Test", refSol(pos, buildCost), result);
    
    return 0;
}