#pragma once

#include <vector>
#include <functional>
#include <limits>
#include <string>

#include "utils.h"


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


template<typename T, typename Compare = std::less<T>>
class ConvexGLWS {
public:
    T compute(const std::vector<T>& data,
              std::function<T(int, int)> costFunc,
              Compare cmp);

private:
    // FindIntervals: In the state interval [il, ir], find the best decision index 
    // from the candidate decision index interval [jl, jr].
    std::vector<Interval> FindIntervals(int jl, int jr, int il, int ir,
                                        const std::vector<T>& dp,
                                        std::function<T(int, int)> costFunc,
                                        Compare cmp) {
        std::vector<Interval> intervals;
        if (il > ir) return intervals;
        if (il == ir) {
            int bestCandidate = jl;
            T bestValue = dp[bestCandidate] + costFunc(bestCandidate, il);
            for (int j = jl + 1; j <= jr; j++) {
                T candidate = dp[j] + costFunc(j, il);
                if (cmp(candidate, bestValue)) {
                    bestValue = candidate;
                    bestCandidate = j;
                }
            }
            intervals.push_back({il, ir, bestCandidate});
            return intervals;
        }
        int im = (il + ir) / 2;
        int bestCandidate = jl;
        T bestValue = dp[bestCandidate] + costFunc(bestCandidate, im);
        for (int j = jl + 1; j <= jr; j++) {
            T candidate = dp[j] + costFunc(j, im);
            if (cmp(candidate, bestValue)) {
                bestValue = candidate;
                bestCandidate = j;
            }
        }

        std::vector<Interval> leftIntervals, rightIntervals;
        // Determine if the problem size is large enough (i.e., the subproblem size exceeds the threshold) 
        // to create tasks, preventing the overhead of overly fine-grained tasks.
        const int THRESHOLD = 20;
        if (ir - il > THRESHOLD) {
            #pragma omp task shared(leftIntervals)
            {
                leftIntervals = FindIntervals(jl, bestCandidate, il, im - 1, dp, costFunc, cmp);
            }
            #pragma omp task shared(rightIntervals)
            {
                rightIntervals = FindIntervals(bestCandidate, jr, im + 1, ir, dp, costFunc, cmp);
            }
            #pragma omp taskwait
        } else {
            // For small subproblems, compute sequentially
            leftIntervals = FindIntervals(jl, bestCandidate, il, im - 1, dp, costFunc, cmp);
            rightIntervals = FindIntervals(bestCandidate, jr, im + 1, ir, dp, costFunc, cmp);
        }
    
        intervals.insert(intervals.end(), leftIntervals.begin(), leftIntervals.end());
        intervals.push_back({im, im, bestCandidate});
        intervals.insert(intervals.end(), rightIntervals.begin(), rightIntervals.end());
        return intervals;
    }

    // Update the compressed best decision array B,
    void UpdateBest(int now, int cordon, int n, std::vector<T>& dp,
                    std::vector<Interval>& B,
                    std::function<T(int, int)> costFunc,
                    Compare cmp) {
        std::vector<Interval> B_new = FindIntervals(now + 1, cordon - 1, cordon, n - 1, dp, costFunc, cmp);
        // Merge B_new with the old B: Keep the part of B_old that covers [0, cordon-1], then append B_new
        std::vector<Interval> B_merged;
        for (const auto &interval : B) {
            if (interval.r < cordon) {
                B_merged.push_back(interval);
            }
        }
        for (const auto &interval : B_new) {
            B_merged.push_back(interval);
        }
        // Simply merge adjacent intervals with the same best decision
        if (!B_merged.empty()) {
            std::vector<Interval> B_compact;
            B_compact.push_back(B_merged[0]);
            for (size_t i = 1; i < B_merged.size(); i++) {
                if (B_merged[i].j == B_compact.back().j && B_merged[i].l == B_compact.back().r + 1) {
                    B_compact.back().r = B_merged[i].r;
                } else {
                    B_compact.push_back(B_merged[i]);
                }
            }
            B = B_compact;
        } else {
            B = B_merged;
        }
    }
};