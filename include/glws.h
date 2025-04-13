template <typename T, typename Compare = std::less<T>>
class ConvexGLWS {
 public:
  T compute(const std::vector<T> &data, std::function<T(int, int)> costFunc, Compare cmp) {
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

 private:
  // FindIntervals: In the state interval [il, ir], find the best decision index
  // from the candidate decision index interval [jl, jr].
  std::vector<Interval> FindIntervals(int jl, int jr, int il, int ir, const std::vector<T> &dp,
                                      std::function<T(int, int)> costFunc, Compare cmp) {
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
      { leftIntervals = FindIntervals(jl, bestCandidate, il, im - 1, dp, costFunc, cmp); }
#pragma omp task shared(rightIntervals)
      { rightIntervals = FindIntervals(bestCandidate, jr, im + 1, ir, dp, costFunc, cmp); }
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
  void UpdateBest(int now, int cordon, int n, std::vector<T> &dp, std::vector<Interval> &B,
                  std::function<T(int, int)> costFunc, Compare cmp) {
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