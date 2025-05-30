#pragma once

#include <functional>
#include <string>
#include <vector>

template <typename T, typename Compare = std::less<T>>
class TournamentTree {
 public:
  TournamentTree(const std::vector<T> &data, Compare cmp = Compare())
      : data_(&data), finalized_(new std::vector<bool>(data.size(), false)), cmp_(cmp) {}

  // Query the index of the smallest data[i] in the range [start, end) that is not in the finalized state
  int query(int start, int end) { return 0; }

  // When the index is finalized, remove the index from the data structure
  void remove(int index) {}

 private:
  const std::vector<T> *data_;
  const std::vector<bool> *finalized_;
  Compare cmp_;
};
