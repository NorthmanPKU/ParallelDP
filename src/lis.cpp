#include "include/lis.h"
#include "include/tournament_tree.h"
#include <algorithm>
#include <omp.h>


template<typename T, typename Compare>
int LIS<T, Compare>::compute(const std::vector<T>& data, Compare cmp) {
    int n = data.size();
    if (n == 0) return 0;
    // dp[i] represents the length of the longest increasing subsequence ending at data[i], initially set to 1
    std::vector<int> dp(n, 1);
    // finalized[i] indicates whether data[i] has been finalized
    std::vector<bool> finalized(n, false);
    // Used to query the index with minimum value in the non-finalized range
    TournamentTree<T, Compare> tree(data, cmp);
    
    int currentRound = 1;
    int numFinalized = 0;
    // Used to record the cordon index of the current round
    int cordonIdx = -1;       
    // Use the Cordon algorithm to process states in rounds until all states are finalized
    while (numFinalized < n) {
        // Find the index of "the smallest element in the current prefix"
        cordonIdx = tree.query(cordonIdx + 1, n);
        if (cordonIdx == -1) break;
        
        dp[cordonIdx] = currentRound;

        #pragma omp parallel for schedule(dynamic)
        for (int i = cordonIdx + 1; i < n; i++) {
            if (!finalized[i] && cmp(data[cordonIdx], data[i])) {
                #pragma omp critical
                {
                    dp[i] = std::max(dp[i], currentRound + 1);
                }
            }
        }

        // Finalize the current state
        finalized[cordonIdx] = true;
        numFinalized++;
        
        tree.remove(cordonIdx);
        currentRound++;
    }
    
    return *std::max_element(dp.begin(), dp.end());
}


template class TournamentTree<int>;
template class TournamentTree<float>;
template class TournamentTree<double>;
template class TournamentTree<long>;
template class TournamentTree<std::string>;

template class LIS<int>;
template class LIS<float>;
template class LIS<double>;
template class LIS<long>;
template class LIS<std::string>;