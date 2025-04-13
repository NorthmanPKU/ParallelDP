#pragma once

#include <vector>

// Used to store compressed information of the best decisions: indicates that for all states within the interval [l, r],
// their current best decision is j
struct Interval {
  int l;
  int r;
  int j;
};

inline int getBest(int j, const std::vector<Interval> &arr) {
  for (const auto &interval : arr) {
    if (j >= interval.l && j <= interval.r) {
      return interval.j;
    }
  }
  return 0;
}