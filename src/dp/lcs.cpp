#include <omp.h>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>
// #include "include/tournament_tree.h"
#include "problem.h"
#include "segment_tree.h"

// 废弃了，之后如果不需要参考了就可以删了

template <typename T, typename Compare>
int LCS<T, Compare>::compute(const std::vector<T> &data1, const std::vector<T> &data2, Compare cmp) {
  int n = data1.size(), m = data2.size();
  if (n == 0 || m == 0) return 0;

  // Get the effective states: (i, j) pairs where data1[i] == data2[j]
  std::unordered_map<T, std::vector<int>> dict;
  for (int j = 0; j < m; j++) {
    dict[data2[j]].push_back(j);
  }
  std::vector<std::pair<int, int>> effectiveStates;
  for (int i = 0; i < n; i++) {
    auto it = dict.find(data1[i]);
    if (it != dict.end()) {
      for (int j : it->second) {
        effectiveStates.push_back({i, j});
      }
    }
  }

  int l = effectiveStates.size();
  if (l == 0) return 0;

  // Primary key is i (index in sequence A) in ascending order,
  // secondary key is j (index in sequence B) in descending order
  std::sort(effectiveStates.begin(), effectiveStates.end(),
            [](const std::pair<int, int> &a, const std::pair<int, int> &b) {
              if (a.first != b.first) return a.first < b.first;
              return a.second > b.second;
            });

  std::vector<int> dp(l, 1);
  std::vector<bool> finalized(l, false);

  auto effComp = [](const std::pair<int, int> &a, const std::pair<int, int> &b) -> bool {
    if (a.second != b.second) return a.second < b.second;
    return a.first > b.first;
  };
  // TournamentTree<std::pair<int,int>, decltype(effComp)> tree(effectiveStates, effComp);
  auto max_pair = std::pair<int, int>(std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
  SegmentTree<std::pair<int, int>> tree(effectiveStates, max_pair);
  tree.print_tree();

  int currentRound = 1;
  int numFinalized = 0;
  int cordonIdx = -1;
  int maxResult = 0;

  int prev_i = -1, prev_j = -1;
  while (numFinalized < l) {
    // cordonIdx = tree.query(cordonIdx + 1, l);
    /** Naive method for test */
    // int cordonIdx = -1;
    // int min_j = 1000000000; // 一个足够大的数
    // for (int i = 0; i < l; i++) {
    //     if (!finalized[i] &&
    //         effectiveStates[i].first > prev_i &&
    //         effectiveStates[i].second > prev_j) {
    //         if (effectiveStates[i].second < min_j) {
    //             cordonIdx = i;
    //             min_j = effectiveStates[i].second;
    //         }
    //     }
    // }

    // if (cordonIdx == -1) break;
    // auto ret_pair = tree.query(cordonIdx + 1, l - 1);
    // cordonIdx = ret_pair.first;
    cordonIdx = tree.find_min_index();
    if (cordonIdx == -1) break;

    dp[cordonIdx] = currentRound;

#pragma omp parallel for schedule(dynamic)
    for (int i = cordonIdx + 1; i < l; i++) {
      if (!finalized[i] && (effectiveStates[cordonIdx].first < effectiveStates[i].first) &&
          (effectiveStates[cordonIdx].second < effectiveStates[i].second)) {
#pragma omp critical
        { dp[i] = std::max(dp[i], dp[cordonIdx] + 1); }
      }
    }

    finalized[cordonIdx] = true;
    numFinalized++;
    maxResult = std::max(maxResult, dp[cordonIdx]);
    std::cout << "cordonIdx: " << cordonIdx << std::endl;
    std::cout << "numFinalized: " << numFinalized << std::endl;
    std::cout << "maxResult: " << maxResult << std::endl;

    tree.remove(cordonIdx);
    // tree.print_tree();
    currentRound++;

    /** Naive method for test */
    prev_i = effectiveStates[cordonIdx].first;
    prev_j = effectiveStates[cordonIdx].second;
  }
  std::cout << "currentRound: " << currentRound << std::endl;

  currentRound = 1;
  numFinalized = 0;
  cordonIdx = -1;
  maxResult = 0;
  prev_i = -1;
  prev_j = -1;

  // while (numFinalized < l) {

  // cordonIdx = tree.query(cordonIdx + 1, l);
  /** Naive method for test */
  int min_j = 1000000000;  // 一个足够大的数
  for (int i = 0; i < l; i++) {
    if (!finalized[i] && effectiveStates[i].first > prev_i && effectiveStates[i].second > prev_j) {
      if (effectiveStates[i].second < min_j) {
        cordonIdx = i;
        min_j = effectiveStates[i].second;
      }
    }
    std::cout << "cordonIdx: " << cordonIdx << std::endl;
  }
  // }

  return maxResult;
}

template class LCS<int>;
template class LCS<float>;
template class LCS<double>;
template class LCS<long>;
template class LCS<char>;
