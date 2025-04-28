#pragma once

#include <chrono>
#include <unordered_map>
#include "lis.h"
#include "segment_tree.h"
#include "segment_tree_cilk.h"
#include <chrono>

#include "parlay/internal/group_by.h"
#include "parlay/parallel.h"
#include "parlay/primitives.h"
#include "utils.h"
#include "segment_tree_cilk_opt.h"

template <typename Left, typename Right>
void conditional_par_do(bool parallel, Left left, Right right) {
  if (parallel) {
    parlay::parallel_do(left, right);
  } else {
    left();
    right();
  }
}

#define lc(x) ((x) << 1)
#define rc(x) ((x) << 1 | 1)

// Use the Cordon algorithm to solve the Longest Common Subsequence (LCS) problem,
// supporting any data type T and user-defined comparison functions.
template <typename T, typename Compare = std::less<T>>
class LCS {
  private:
    std::unique_ptr<Tree<int>> tree;
    std::unique_ptr<SegmentTreeCilkOpt<size_t>> tree_opt;
 public:

  /**
   * @brief Conver lcs to lis to compute
   */
  int compute_as_lis(const std::vector<T> &data1, const std::vector<T> &data2, bool parallel = false,
                     int granularity = 0) {
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

    return lis.compute(effectiveStates, parallel, granularity, effComp, max_pair);
  }

  int compute_as_lis(const std::string &data1, const std::string &data2, bool parallel = false, int granularity = 0) {
    return compute_as_lis(std::vector<T>(data1.begin(), data1.end()), std::vector<T>(data2.begin(), data2.end()),
                          parallel, granularity);
  }

  int compute_arrows(std::vector<std::vector<int>> &arrows, ParallelArch arch=ParallelArch::CILK, bool parallel=false, int granularity=0) {
    auto start = std::chrono::high_resolution_clock::now();
    if (arch == ParallelArch::CILK) {
      tree = std::make_unique<SegmentTreeCilk<int>>(arrows, std::numeric_limits<int>::max(), parallel, granularity);
    } else if (arch == ParallelArch::OPENMP) {
      tree = std::make_unique<SegmentTreeOpenMP<int>>(arrows, std::numeric_limits<int>::max(), parallel, granularity);
    } else {
      throw std::invalid_argument("Invalid parallel architecture");
    }
    auto end = std::chrono::high_resolution_clock::now();
    // std::cout << ", Tree building time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    int round = 0;
    while (tree->global_min() < std::numeric_limits<int>::max()) {
      round++;
      tree->prefix_min();
    }
    auto end2 = std::chrono::high_resolution_clock::now();
    std::cout << "LCS time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end2 - end).count() << "ms" << std::endl;

    return round;
  }

  int compute_arrows_paralay(size_t n, const parlay::sequence<parlay::sequence<size_t>>& arrows, bool ifparallel=false, int granularity=5000) {
    const size_t inf = std::numeric_limits<size_t>::max();
    parlay::sequence<size_t> now(n + 1);

    auto Read = [&](size_t i) {
      if (now[i] >= arrows[i].size()) return inf;
      return arrows[i][now[i]];
    };

    parlay::sequence<size_t> tree(4 * n);

    std::function<void(size_t, size_t, size_t)> Construct =
      [&](size_t x, size_t l, size_t r) {
        if (l == r) {
          tree[x] = Read(l);
          return;
        }
        size_t mid = (l + r) / 2;
        bool parallel = ifparallel && r - l > granularity;
        conditional_par_do(
            parallel, [&]() { Construct(lc(x), l, mid); },
            [&]() { Construct(rc(x), mid + 1, r); });
        tree[x] = std::min(tree[lc(x)], tree[rc(x)]);
      };

      std::function<void(size_t, size_t, size_t, size_t)> PrefixMin =
      [&](size_t x, size_t l, size_t r, size_t pre) {
        if (tree[x] > pre) return;
        if (l == r) {
          auto& ys = arrows[l];
          if (now[l] + 8 >= ys.size() || ys[now[l] + 8] > pre) {
            while (now[l] < ys.size() && ys[now[l]] <= pre) {
              now[l]++;
            }
          } else {
            now[l] = std::upper_bound(ys.begin() + now[l], ys.end(), pre) -
                     ys.begin();
          }
          tree[x] = Read(l);
          return;
        }
        size_t mid = (l + r) / 2;
        if (tree[x] == tree[rc(x)]) {
          if (tree[lc(x)] <= pre && tree[lc(x)] < inf) {
            bool parallel = ifparallel && r - l > granularity;
            size_t lc_val = tree[lc(x)];
            conditional_par_do(
                parallel, [&]() { PrefixMin(lc(x), l, mid, pre); },
                [&]() { PrefixMin(rc(x), mid + 1, r, lc_val); });
          } else {
            PrefixMin(rc(x), mid + 1, r, pre);
          }
        } else {
          PrefixMin(lc(x), l, mid, pre);
        }
        tree[x] = std::min(tree[lc(x)], tree[rc(x)]);
      };

    Construct(1, 1, n);
    size_t round = 0;
    auto start = std::chrono::high_resolution_clock::now();
    while (tree[1] < inf) {
      round++;
      PrefixMin(1, 1, n, inf);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "LCS time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
    return round;
  }

  int compute_arrows_opt(const parlay::sequence<parlay::sequence<size_t>>& arrows, bool ifparallel=false, int granularity=5000) {
    auto start = std::chrono::high_resolution_clock::now();
    tree_opt = std::make_unique<SegmentTreeCilkOpt<size_t>>(arrows, std::numeric_limits<size_t>::max(), ifparallel, granularity);
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Prepare time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms"
              << std::endl;

    int round = 0;
    while (tree_opt->global_min() < std::numeric_limits<int>::max()) {
      round++;
      tree_opt->prefix_min();
    }
    auto end2 = std::chrono::high_resolution_clock::now();
    std::cout << "LCS time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end2 - end).count() << "ms" << std::endl;

    return round;
  }

  // without using arrows
  int compute(const std::vector<T> &data1, const std::vector<T> &data2, ParallelArch arch=ParallelArch::CILK, bool parallel=false, int granularity=0) {
    auto start = std::chrono::high_resolution_clock::now();
    int n = data1.size(), m = data2.size();
    if (n == 0 || m == 0) return 0;

    std::vector<std::vector<int>> arrows(n, std::vector<int>(0));

    std::unordered_map<T, std::vector<int>> data2_to_indices;
    for (int j = 0; j < m; j++) {
      data2_to_indices[data2[j]].push_back(j);
    }

    // Get the effective states: (i, j) pairs where data1[i] == data2[j]
    for (int i = 0; i < n; i++) {
      auto it = data2_to_indices.find(data1[i]);
      if (it != data2_to_indices.end()) {
        for (int j : it->second) {
          arrows[i].push_back(j);
        }
      }
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Prepare time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms"
              << std::endl;

    return compute_arrows(arrows, arch, parallel, granularity);
  }

  int compute(const std::string &data1, const std::string &data2, ParallelArch arch=ParallelArch::CILK, bool parallel=false, int granularity=0) {
    return compute(std::vector<T>(data1.begin(), data1.end()), std::vector<T>(data2.begin(), data2.end()), arch, parallel, granularity);
  }
};