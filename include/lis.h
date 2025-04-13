#pragma once

#include "segment_tree.h"

// Use the Cordon algorithm to solve the Longest Increasing Subsequence (LIS) problem, 
// supporting any data type T and user-defined comparison functions.
template<typename T, typename Compare = std::less<T>>
class LIS {
public:
    // The parameter cmp is a comparison function, defaulting to std::less<T>
    int compute(const std::vector<T>& data, Compare cmp = Compare(), T inf_value = std::numeric_limits<T>::max()) {
        int n = data.size();
        if (n == 0) return 0;
        // dp[i] represents the length of the longest increasing subsequence ending at data[i], initially set to 1
        std::vector<int> dp(n, 1);
        // finalized[i] indicates whether data[i] has been finalized
        std::vector<bool> finalized(n, false);
        // Used to query the index with minimum value in the non-finalized range
        SegmentTree<T> tree(data, inf_value);
        tree.print_tree();
        int numFinalized = 0;
        // Used to record the cordon index of the current round
        int cordonIdx = -1;       
        int maxResult = 0;
        // Use the Cordon algorithm to process states in rounds until all states are finalized
        while (numFinalized < n) {
            // Find the index of "the smallest element in the current prefix"
            cordonIdx = tree.find_min_index();
            if (cordonIdx == -1) break;
            
            #pragma omp parallel for schedule(dynamic)
            for (int i = cordonIdx + 1; i < n; i++) {
                if (!finalized[i] && cmp(data[cordonIdx], data[i])) {
                    #pragma omp critical
                    {
                        dp[i] = std::max(dp[i], dp[cordonIdx] + 1);
                    }
                }
            }
    
            // Finalize the current state
            finalized[cordonIdx] = true;
            numFinalized++;
            maxResult = std::max(maxResult, dp[cordonIdx]);
            
            tree.remove(cordonIdx);
        }

        return maxResult;
    }
};