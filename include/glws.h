#pragma once

#include <omp.h>
#include <functional>
#include <limits>
#include <unordered_map>
#include <vector>
#include "utils.h"

template <typename T, typename Compare = std::less<T>>
class ConvexGLWS {
 public:
  // Assume E[i] = D[i]
  T compute(const std::vector<T> &data, std::function<T(int, int, const std::vector<T> &)> costFunc,
            Compare cmp = Compare()) {
    int n = data.size();
    if (n == 0) return T();

    std::vector<T> dp(n, std::numeric_limits<T>::max());
    dp[0] = 0;

    std::vector<bool> finalized(n, false);
    finalized[0] = true;

    // TournamentTree<T, Compare> tree(dp, cmp);
    // tree.remove(0);

    std::vector<Interval> optimal_decisions;
    if (n > 1) optimal_decisions.push_back({1, n - 1, 0});

    int now = 0;
    while (now < n - 1) {
      // int cordon = tree.query(now + 1, n);
      // TEST
      int cordon = -1;
      for (int i = now + 1; i < n; ++i) {
        if (!finalized[i]) {
          cordon = i;
          break;
        }
      }
      if (cordon == -1) break;

      finalized[cordon] = true;
#pragma omp parallel for schedule(dynamic)
      for (int i = cordon + 1; i < n; i++) {
        if (!finalized[i]) {
          T newCost = dp[cordon] + costFunc(cordon, i, data);
          if (cmp(newCost, dp[i])) {
#pragma omp critical
            { dp[i] = std::min(dp[i], newCost); }
          }
        }
      }

      // tree.remove(cordon);

      UpdateBest(now, cordon, n, dp, optimal_decisions, costFunc, cmp, data);
      now = cordon;
    }

    return dp[n - 1];
  }

 private:
  std::vector<Interval> FindIntervals(int jl, int jr, int il, int ir, const std::vector<T> &dp,
                                      std::function<T(int, int, const std::vector<T> &)> costFunc, Compare cmp,
                                      const std::vector<T> &data) {
    std::vector<Interval> intervals;
    if (il > ir) return intervals;
    if (il == ir) {
      int bestCandidate = jl;
      T bestValue = dp[bestCandidate] + costFunc(bestCandidate, il, data);
      for (int j = jl + 1; j <= jr; j++) {
        T candidateValue = dp[j] + costFunc(j, il, data);
        if (cmp(candidateValue, bestValue)) {
          bestValue = candidateValue;
          bestCandidate = j;
        }
      }
      intervals.push_back({il, ir, bestCandidate});
      return intervals;
    }
    int im = (il + ir) / 2;
    int bestCandidate = jl;
    T bestValue = dp[bestCandidate] + costFunc(bestCandidate, im, data);
    for (int j = jl + 1; j <= jr; j++) {
      T candidateValue = dp[j] + costFunc(j, im, data);
      if (cmp(candidateValue, bestValue)) {
        bestValue = candidateValue;
        bestCandidate = j;
      }
    }

    std::vector<Interval> leftIntervals, rightIntervals;
    const int THRESHOLD = 20;
    if (ir - il > THRESHOLD) {
#pragma omp task shared(leftIntervals)
      { leftIntervals = FindIntervals(jl, bestCandidate, il, im - 1, dp, costFunc, cmp, data); }
#pragma omp task shared(rightIntervals)
      { rightIntervals = FindIntervals(bestCandidate, jr, im + 1, ir, dp, costFunc, cmp, data); }
#pragma omp taskwait
    } else {
      leftIntervals = FindIntervals(jl, bestCandidate, il, im - 1, dp, costFunc, cmp, data);
      rightIntervals = FindIntervals(bestCandidate, jr, im + 1, ir, dp, costFunc, cmp, data);
    }

    intervals.insert(intervals.end(), leftIntervals.begin(), leftIntervals.end());
    intervals.push_back({im, im, bestCandidate});
    intervals.insert(intervals.end(), rightIntervals.begin(), rightIntervals.end());
    return intervals;
  }

  void UpdateBest(int now, int cordon, int n, std::vector<T> &dp, std::vector<Interval> &B,
                  std::function<T(int, int, const std::vector<T> &)> costFunc, Compare cmp,
                  const std::vector<T> &data) {
    std::vector<Interval> B_new = FindIntervals(now + 1, cordon - 1, cordon, n - 1, dp, costFunc, cmp, data);
    std::vector<Interval> B_merged;
    for (const auto &interval : B) {
      if (interval.r < cordon) {
        B_merged.push_back(interval);
      }
    }
    for (const auto &interval : B_new) {
      B_merged.push_back(interval);
    }
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
