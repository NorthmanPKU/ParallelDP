#include <omp.h>
#include <algorithm>
#include "problem.h"
#include "segment_tree.h"

// 废弃了，之后如果不需要参考了就可以删了

template <typename T, typename Compare>
int LIS<T, Compare>::compute(const std::vector<T> &data, Compare cmp) {
  int n = data.size();
  if (n == 0) return 0;
  // dp[i] represents the length of the longest increasing subsequence ending at data[i], initially set to 1
  std::vector<int> dp(n, 1);
  // finalized[i] indicates whether data[i] has been finalized
  std::vector<bool> finalized(n, false);
  // Used to query the index with minimum value in the non-finalized range
  // TournamentTree<T, Compare> tree(data, cmp);
  SegmentTree<T> tree(data);
  tree.print_tree();
  int numFinalized = 0;
  // Used to record the cordon index of the current round
  int cordonIdx = -1;
  int maxResult = 0;
  // Use the Cordon algorithm to process states in rounds until all states are finalized
  while (numFinalized < n) {
    // Find the index of "the smallest element in the current prefix"
    // cordonIdx = tree.query(cordonIdx + 1, n - 1);
    cordonIdx = tree.find_min_index();

    if (cordonIdx == -1) break;

#pragma omp parallel for schedule(dynamic)
    for (int i = cordonIdx + 1; i < n; i++) {
      if (!finalized[i] && cmp(data[cordonIdx], data[i])) {
#pragma omp critical
        { dp[i] = std::max(dp[i], dp[cordonIdx] + 1); }
      }
    }

    // Finalize the current state
    finalized[cordonIdx] = true;
    numFinalized++;
    maxResult = std::max(maxResult, dp[cordonIdx]);

    tree.remove(cordonIdx);
  }

  // printf("tree max result: %d\n", maxResult);

  // dp = std::vector<int>(n, 1);
  // finalized = std::vector<bool>(n, false);
  // numFinalized = 0;
  // Used to record the cordon index of the current round
  // cordonIdx = -1;
  // maxResult = 0;
  // while (numFinalized < n) {
  //     // Find the index of "the smallest element in the current prefix"
  //     // cordonIdx = tree.query(cordonIdx + 1, n - 1);
  //     // cordonIdx = tree.query_index(cordonIdx + 1, n - 1);
  //     /** Naive method for test, should use the tree above if implemented */
  //     cordonIdx = -1;
  //     for (int i = 0; i < n; i++) {
  //         if (!finalized[i] && (cordonIdx == -1 || cmp(data[i], data[cordonIdx]))) {
  //             cordonIdx = i;
  //         }
  //     }
  //     printf("naive cordonIdx: %d\n", cordonIdx);

  //     if (cordonIdx == -1) break;

  //     #pragma omp parallel for schedule(dynamic)
  //     for (int i = cordonIdx + 1; i < n; i++) {
  //         if (!finalized[i] && cmp(data[cordonIdx], data[i])) {
  //             #pragma omp critical
  //             {
  //                 dp[i] = std::max(dp[i], dp[cordonIdx] + 1);
  //             }
  //         }
  //     }

  //     // Finalize the current state
  //     finalized[cordonIdx] = true;
  //     numFinalized++;
  //     maxResult = std::max(maxResult, dp[cordonIdx]);

  //     // tree.remove(cordonIdx);
  // }

  printf("naive max result: %d\n", maxResult);

  return maxResult;
}

template class LIS<int>;
template class LIS<float>;
template class LIS<double>;
template class LIS<long>;
template class LIS<std::string>;

template class LIS<int, std::greater<int>>;