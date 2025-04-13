#include <omp.h>
#include <algorithm>
#include "include/problem.h"
#include "include/tournament_tree.h"
#include "include/utils.h"

// 废弃了，之后如果不需要参考了就可以删了

template <typename T, typename Compare>
T ConvexGLWS<T, Compare>::compute(const std::vector<T> &data, std::function<T(int, int)> costFunc, Compare cmp) {
  int n = data.size();
  if (n == 0) return T();

  std::vector<T> dp(n, std::numeric_limits<T>::max());
  dp[0] = 0;

  std::vector<bool> finalized(n, false);
  finalized[0] = true;
  int numFinalized = 1;

  TournamentTree<T, Compare> tree(dp, cmp);
  tree.remove(0);

  // Compress the optimal decision array. Initially, for states 1 to n-1, the default optimal decision is 0
  std::vector<Interval> optimal_decisions;
  if (n > 1) optimal_decisions.push_back({1, n - 1, 0});

  int now = 0;
  while (now < n - 1) {
    int cordon = tree.query(now + 1, n);
    if (cordon == -1) break;

    finalized[cordon] = true;
#pragma omp parallel for schedule(dynamic)
    for (int i = cordon + 1; i < n; i++) {
      if (!finalized[i]) {
        T newCost = dp[cordon] + costFunc(cordon, i);
        if (cmp(newCost, dp[i])) {
#pragma omp critical
          { dp[i] = std::min(dp[i], newCost); }
        }
      }
    }

    tree.remove(cordon);

    UpdateBest(now, cordon, n, dp, optimal_decisions, costFunc, cmp);
    now = cordon;
  }

  return dp[n - 1];
}

template class ConvexGLWS<int, std::less<int>>;
template class ConvexGLWS<float, std::less<float>>;
template class ConvexGLWS<double, std::less<double>>;