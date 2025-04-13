#pragma once

#include <unordered_map>
#include "lis.h"
#include "segment_tree.h"

// Use the Cordon algorithm to solve the Longest Common Subsequence (LCS) problem,
// supporting any data type T and user-defined comparison functions.
template <typename T, typename Compare = std::less<T>>
class LCS {
 public:
  /**
   * @brief Conver lcs to lis to compute
   */
  int compute(const std::vector<T> &data1, const std::vector<T> &data2) {
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

    // Primary key is i (index in sequence A) in ascending order,
    // secondary key is j (index in sequence B) in descending order
    std::sort(effectiveStates.begin(), effectiveStates.end(),
              [](const std::pair<int, int> &a, const std::pair<int, int> &b) {
                if (a.first != b.first) return a.first < b.first;
                return a.second > b.second;
              });

    auto effComp = [](const std::pair<int, int> &a, const std::pair<int, int> &b) -> bool {
      return a.first < b.first && a.second < b.second;
    };
    auto max_pair = std::pair<int, int>(std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
    LIS<std::pair<int, int>, decltype(effComp)> lis;

    return lis.compute(effectiveStates, effComp, max_pair);
  }

  int compute(const std::string &data1, const std::string &data2) {
    return compute(std::vector<T>(data1.begin(), data1.end()), std::vector<T>(data2.begin(), data2.end()));
  }
};