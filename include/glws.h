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
    std::vector<long double> pos;
    pos.reserve(data.size() + 1);
    pos.push_back(0);
    pos.insert(pos.end(), data.begin(), data.end());

    int n = data.size();
    if (n == 0) return T();

    std::vector<T> D(n + 1, std::numeric_limits<T>::max());
    D[0] = 0;
    int now = 0;

    std::vector<Interval> B;
    B.push_back({1, n, 0});

    while (now < n) {
      int cordon = findCordon(now, D, B, costFunc, cmp, pos);
#pragma omp parallel for
      for (int i = now + 1; i < cordon; ++i) {
        int b = findBest(i, B);
        D[i] = D[b] + costFunc(b, i, pos);
      }
      printf("now: %d\n", now);
      printf("Cordon: %d\n", cordon);
      updateBest(now, cordon, n, D, B, costFunc, cmp, pos);
      std::cout << "D: ";
      for (const auto &d : D) {
        std::cout << d << " ";
      }
      std::cout << std::endl;
      now = cordon - 1;
      printf("------------------------\n");
    }

    return D[n];
  }

 private:
  int findCordon(int now, std::vector<T> &D, const std::vector<Interval> &B,
                 std::function<T(int, int, const std::vector<T> &)> costFunc, Compare cmp, const std::vector<T> &data) {
    int n = data.size() - 1;
    int cordon = n + 1;
    for (int t = 1; (now + (1 << t)) <= n; ++t) {
      int l = now + (1 << (t - 1));
      int r = std::min(n, now + (1 << t) - 1);
      printf("l: %d, r: %d\n", l, r);
      std::vector<int> s_j(r - l + 1, n + 1);
      // #pragma omp parallel for schedule(dynamic)
      for (int j = l; j <= r; ++j) {
        int bestj = findBest(j, B);
        printf("j: %d, bestj: %d\n", j, bestj);
        T Ej = D[bestj] + costFunc(bestj, j, data);
        T Dj = Ej;
        if (cmp(Dj, D[j])) {
          printf("Updating D[%d]: %Lf\n", j, Dj);
          // Relax j
          // Now find the earliest state i > j s.t. j can relax i better than its current best
          for (int i = j + 1; i <= n; ++i) {
            int current_best = findBest(i, B);
            T current_val = D[current_best] + costFunc(current_best, i, data);
            T candidate_val = Dj + costFunc(j, i, data);
            std::cout << "D[current_best]: " << D[current_best]
                      << " + costFunc(current_best, j, data): " << costFunc(current_best, j, data) << std::endl;
            std::cout << "Dj: " << Dj << " + costFunc(j, i, data): " << costFunc(j, i, data) << std::endl;
            if (cmp(candidate_val, current_val)) {
#pragma omp critical
              { s_j[j - l] = i; }
              printf("Relaxing j: %d, i: %d\n", j, i);
              break;
            }
          }
        } else {
          s_j[j - l] = n + 1;
        }
      }
      for (int x : s_j) cordon = std::min(cordon, x);
      if (cordon <= r + 1) break;
    }
    return cordon;
  }

  void updateBest(int now, int cordon, int n, std::vector<T> &D, std::vector<Interval> &B,
                  std::function<T(int, int, const std::vector<T> &)> costFunc, Compare cmp,
                  const std::vector<T> &data) {
    std::vector<Interval> B_new = findIntervals(now + 1, cordon - 1, cordon, n, D, costFunc, cmp, data);
    std::vector<Interval> merged;
    for (const auto &interval : B)
      if (interval.r < cordon) merged.push_back(interval);
    for (const auto &interval : B_new) merged.push_back(interval);

    std::vector<Interval> compact;
    if (!merged.empty()) {
      compact.push_back(merged[0]);
      for (size_t i = 1; i < merged.size(); i++) {
        if (merged[i].j == compact.back().j && merged[i].l == compact.back().r + 1) {
          compact.back().r = merged[i].r;
        } else {
          compact.push_back(merged[i]);
        }
      }
    }
    B = compact;
    std::cout << "B after updateBest: ";
    for (const auto &interval : B) {
      std::cout << "[" << interval.l << "," << interval.r << "]→" << interval.j << " ";
    }
    std::cout << std::endl;
  }

  std::vector<Interval> findIntervals(int jl, int jr, int il, int ir, const std::vector<T> &D,
                                      std::function<T(int, int, const std::vector<T> &)> costFunc, Compare cmp,
                                      const std::vector<T> &data) {
    std::vector<Interval> result;
    if (il > ir) return result;
    if (il == ir) {
      int best = jl;
      T val = D[best] + costFunc(best, il, data);
      for (int j = jl + 1; j <= jr; ++j) {
        T cand = D[j] + costFunc(j, il, data);
        if (cmp(cand, val)) {
          val = cand;
          best = j;
        }
      }
      result.push_back({il, ir, best});
      return result;
    }
    int im = (il + ir) / 2;
    int best = jl;
    T val = D[best] + costFunc(best, im, data);
    for (int j = jl + 1; j <= jr; ++j) {
      T cand = D[j] + costFunc(j, im, data);
      if (cmp(cand, val)) {
        val = cand;
        best = j;
      }
    }
    auto L = findIntervals(jl, best, il, im - 1, D, costFunc, cmp, data);
    auto R = findIntervals(best, jr, im + 1, ir, D, costFunc, cmp, data);
    result.insert(result.end(), L.begin(), L.end());
    result.push_back({im, im, best});
    result.insert(result.end(), R.begin(), R.end());
    return result;
  }
};
