#pragma once

#include <omp.h>
#include <algorithm>
#include <atomic>
#include <cassert>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
template <typename T1, typename T2>
std::ostream &operator<<(std::ostream &os, const std::pair<T1, T2> &p) {
  os << "(" << p.first << "," << p.second << ")";
  return os;
}

/**
 * @brief A comprehensive segment tree implementation supporting both sequential and parallel builds.
 *
 * This segment tree supports operations like finding minimums over ranges and performing
 * prefix minimum operations on sequences. It can be built in both single-threaded and
 * parallel modes for improved performance on large datasets.
 *
 * @tparam T The data type stored in the segment tree (must support comparison)
 */
template <typename T>
class SegmentTree {
 private:
  std::vector<T> tree;       // The segment tree array
  size_t n;                  // Number of leaf nodes
  T infinity;                // Value representing infinity
  bool constructed = false;  // Flag to track if tree has been constructed

  // For LCS prefix minimum operation
  bool prefix_mode = false;
  std::vector<size_t> now;
  std::vector<std::vector<T>> arrows;
  int granularity = 1000;
  bool parallel = false;

  // std::atomic<int> task_count{0};

  /**
   * @brief Get the index of the left child of a node
   * @param x The index of the parent node
   * @return The index of the left child
   */
  inline size_t lc(size_t x) const { return 2 * x + 1; }

  /**
   * @brief Get the index of the right child of a node
   * @param x The index of the parent node
   * @return The index of the right child
   */
  inline size_t rc(size_t x) const { return 2 * x + 2; }

  /**
   * @brief Get the index of the parent node
   * @param x The index of a child node
   * @return The index of the parent node
   */
  inline size_t parent(size_t x) const { return (x - 1) / 2; }

  /**
   * @brief Internal recursive function to build the segment tree
   *
   * @param arr Reference to the input data array
   * @param x Current node index in the segment tree
   * @param l Left boundary of the current segment
   * @param r Right boundary of the current segment
   * @param parallel Whether to build the tree in parallel
   * @param granularity The minimum size of a subtree to process in parallel
   */
  void build_recursive(const std::vector<T> &arr, size_t x, size_t l, size_t r) {
    if (l == r) {
      tree[x] = (l < arr.size()) ? arr[l] : infinity;
      return;
    }

    size_t mid = (l + r) / 2;
    bool do_parallel = parallel && (r - l > granularity);

    if (do_parallel) {
#pragma omp task shared(arr, tree)
      { build_recursive(arr, lc(x), l, mid); }
#pragma omp task shared(arr, tree)
      { build_recursive(arr, rc(x), mid + 1, r); }
#pragma omp taskwait
    } else {
      build_recursive(arr, lc(x), l, mid);
      build_recursive(arr, rc(x), mid + 1, r);
    }

    tree[x] = std::min(tree[lc(x)], tree[rc(x)]);
  }

  /**
   * @brief Internal recursive function to query the minimum value in a range
   *
   * @param x Current node index in the segment tree
   * @param l Left boundary of the current segment
   * @param r Right boundary of the current segment
   * @param ql Left boundary of the query range
   * @param qr Right boundary of the query range
   * @return The minimum value in the query range
   */
  T query_recursive(size_t x, size_t l, size_t r, size_t ql, size_t qr) const {
    // If the current segment is completely outside the query range
    if (r < ql || l > qr) {
      return infinity;
    }

    // If the current segment is completely inside the query range
    if (l >= ql && r <= qr) {
      return tree[x];
    }

    // If the current segment is partially inside the query range
    size_t mid = (l + r) / 2;
    T left_min = query_recursive(lc(x), l, mid, ql, qr);
    T right_min = query_recursive(rc(x), mid + 1, r, ql, qr);
    return std::min(left_min, right_min);
  }

  /**
   * @brief Internal function to update a single value in the segment tree
   *
   * @param x Current node index in the segment tree
   * @param l Left boundary of the current segment
   * @param r Right boundary of the current segment
   * @param pos Position to update
   * @param new_val New value to set
   */
  void update_recursive(size_t x, size_t l, size_t r, size_t pos, T new_val) {
    // If we've reached the leaf node
    // std::cout << "update_recursive: x = " << x << ", l = " << l << ", r = " << r << ", pos = " << pos << ", new_val =
    // " << new_val << std::endl;
    if (l == r) {
      tree[x] = new_val;
      return;
    }

    size_t mid = (l + r) / 2;
    if (pos <= mid) {
      update_recursive(lc(x), l, mid, pos, new_val);
    } else {
      update_recursive(rc(x), mid + 1, r, pos, new_val);
    }

    tree[x] = std::min(tree[lc(x)], tree[rc(x)]);
    // std::cout << "update_recursive: tree[x] = " << tree[x] << std::endl;
    // std::cout << "left_child: " << tree[lc(x)] << ", right_child: " << tree[rc(x)] << std::endl;
  }

  /**
   * @brief Perform prefix minimum operation as specified in the provided code
   *
   * @param x Current node index in the segment tree
   * @param l Left boundary of the current segment
   * @param r Right boundary of the current segment
   * @param pre The prefix value
   * @param arrows The array of arrow sequences
   * @param now The current indices in the arrow sequences
   * @param parallel Whether to perform the operation in parallel
   * @param granularity The minimum size of a subtree to process in parallel
   */
  void prefix_min_recursive(size_t x, size_t l, size_t r, T pre) {
    // Early return if this node's value is already greater than pre
    // std::cout << "prefix_min_recursive: x = " << x << ", l = " << l << ", r = " << r << ", pre = " << pre;
    if (tree[x] > pre) {
      return;
    }

    // If we've reached a leaf node
    if (l == r) {
      const auto &ys = arrows[l];

      // Check if we need to do binary search or linear search
      if (now[l] + 8 >= ys.size() || ys[now[l] + 8] > pre) {
        // Linear search (small range)
        while (now[l] < ys.size() && ys[now[l]] <= pre) {
          now[l]++;
        }
      } else {
        // Binary search (large range)
        now[l] = std::upper_bound(ys.begin() + now[l], ys.end(), pre) - ys.begin();
      }

      // Update the tree value after the index change
      tree[x] = read(l);
      return;
    }

    size_t mid = (l + r) / 2;

    // Optimize the traversal based on which child has the minimum value
    if (tree[x] == tree[rc(x)]) {
      if (tree[lc(x)] <= pre && tree[lc(x)] < infinity) {
        bool do_parallel = (r - l > granularity) && parallel;
        T lc_val = tree[lc(x)];

        if (do_parallel) {
          // task_count.fetch_add(1);
          // std::cout << "do_parallel" << std::endl;
          // #pragma omp single
          // {
          //   printf("Current working threads: %d\n", omp_get_num_threads());
          // }
#pragma omp task
          { prefix_min_recursive(lc(x), l, mid, pre); }
          // #pragma omp task
          { prefix_min_recursive(rc(x), mid + 1, r, lc_val); }
#pragma omp taskwait
        } else {
          // std::cout << "do_not_parallel: " << "l: " << l << ", mid: " << mid << ", r: " << r << std::endl;
          prefix_min_recursive(lc(x), l, mid, pre);
          prefix_min_recursive(rc(x), mid + 1, r, lc_val);
        }
      } else {
        // std::cout << "only right" << std::endl;
        prefix_min_recursive(rc(x), mid + 1, r, pre);
      }
    } else {
      // std::cout << "only left" << std::endl;
      prefix_min_recursive(lc(x), l, mid, pre);
    }

    // Update this node's value
    tree[x] = std::min(tree[lc(x)], tree[rc(x)]);
  }

 public:
  /**
   * @brief Construct a new Segment Tree object based on an array
   *
   * @param arr The input array to build the tree from
   * @param inf_value The value to use as infinity (default: maximum value of type T)
   */
  SegmentTree(const std::vector<T> &arr, T inf_value = std::numeric_limits<T>::max(), bool parallel = false,
              size_t granularity = 1000)
      : n(arr.size()), infinity(inf_value), prefix_mode(false), granularity(granularity), parallel(parallel) {
    std::cout << "SegmentTree init" << std::endl;
    // std::cout << "arr: " << std::endl;
    // for (size_t i = 0; i < arr.size(); ++i) {
    //   std::cout << arr[i] << " ";
    // }
    std::cout << std::endl;
    std::cout << "inf_value: " << inf_value << std::endl;
    std::cout << "parallel: " << parallel << std::endl;
    std::cout << "granularity: " << granularity << std::endl;
    if (arr.empty()) {
      throw std::invalid_argument("Input array cannot be empty");
    }
    if constexpr (std::is_same_v<T, std::string>) {
      // TODO: A current workaround for string comparison
      infinity = "zzzzzzzzzzzzzzzzzzzz";
    }
    tree.resize(4 * n, inf_value);
    build(arr);
  }

  /**
   * @brief Construct a new Segment Tree for LCS prefix minimum operation
   *
   * @param _arrows The input array of arrow sequences
   * @param inf_value The value to use as infinity (default: maximum value of type T)
   * @param parallel Whether to build the tree in parallel
   * @param granularity The minimum size of a subtree to process in parallel
   */
  SegmentTree(std::vector<std::vector<T>> _arrows, T inf_value = std::numeric_limits<T>::max(), bool _parallel = false,
              size_t _granularity = 1000)
      : n(_arrows.size()),
        infinity(inf_value),
        arrows(_arrows),
        prefix_mode(true),
        granularity(_granularity),
        parallel(_parallel) {
    std::cout << "SegmentTree Prefix mode init" << std::endl;
    std::cout << "inf_value: " << inf_value << std::endl;
    std::cout << "parallel: " << parallel << std::endl;
    std::cout << "granularity: " << granularity << std::endl;
    if constexpr (std::is_same_v<T, std::string>) {
      // TODO: A current workaround for string comparison
      infinity = "zzzzzzzzzzzzzzzzzzzz";
    }
    tree.resize(4 * n, inf_value);
    now.resize(n, 0);
    std::vector<T> arr(n);
    for (size_t i = 0; i < n; ++i) {
      arr[i] = read(i);
    }
    build(arr);
  }

  /**
   * @brief Print the segment tree in a tree-like structure
   *
   * @param max_depth Maximum depth to print (default: 5, to avoid excessive output for large trees)
   * @param show_indices Whether to show node indices along with values (default: true)
   */
  void print_tree(int max_depth = 5, bool show_indices = true) const {
    if (!constructed) {
      std::cout << "Tree not constructed" << std::endl;
      return;
    }

    std::cout << "Segment Tree Visualization (n=" << n << ", max_depth=" << max_depth << "):" << std::endl;
    print_subtree(0, 0, 0, n - 1, 0, max_depth, "  ", show_indices);
  }

 private:
  /**
   * @brief Recursively print a subtree with proper indentation
   *
   * @param x Current node index
   * @param depth Current depth in the tree
   * @param l Left bound of current segment
   * @param r Right bound of current segment
   * @param current_depth Current depth in the recursion
   * @param max_depth Maximum depth to print
   * @param indent Current indentation string
   * @param show_indices Whether to show node indices along with values
   */
  void print_subtree(size_t x, size_t depth, size_t l, size_t r, int current_depth, int max_depth,
                     const std::string &indent, bool show_indices) const {
    // Print current node
    std::cout << indent;
    if (depth > 0) {
      std::cout << "├─ ";
    }

    // Show node value and optional info
    if (show_indices) {
      std::cout << "[" << x << "] ";
    }

    if (tree[x] == infinity) {
      std::cout << "∞";  // Use infinity symbol for infinity values
    } else {
      std::cout << tree[x];
    }

    std::cout << " (" << l << ":" << r << ")";

    // Mark leaf nodes
    if (l == r) {
      std::cout << " [LEAF]";
    }
    std::cout << std::endl;

    // Stop if we've reached a leaf or maximum depth
    if (l == r || current_depth >= max_depth) {
      if (l != r && current_depth >= max_depth) {
        std::cout << indent << "└─ ... (subtree omitted due to depth limit)" << std::endl;
      }
      return;
    }

    // Calculate middle point
    size_t mid = (l + r) / 2;

    // Print left child
    print_subtree(lc(x), depth + 1, l, mid, current_depth + 1, max_depth, indent + "│  ", show_indices);

    // Print right child
    print_subtree(rc(x), depth + 1, mid + 1, r, current_depth + 1, max_depth, indent + "   ", show_indices);
  }

 public:
  /**
   * @brief Get the size of the segment tree
   *
   * @return The number of leaf nodes in the segment tree
   */
  size_t size() const { return n; }

  /**
   * @brief Get the value at a specific position in the tree
   *
   * @param index The position to access
   * @return The value at the given position
   * @throws std::out_of_range if the index is out of bounds
   */
  T at(size_t index) const {
    if (index >= tree.size()) {
      throw std::out_of_range("Index out of bounds: " + std::to_string(index) + " >= " + std::to_string(tree.size()));
    }
    return tree[index];
  }

  /**
   * Return the current global minimum value in the tree
   */
  T global_min() const { return tree[0]; }

  /**
   * @brief Get the raw tree array
   *
   * @return A const reference to the tree array
   */
  const std::vector<T> &get_tree() const { return tree; }

  /**
   * @brief Build the segment tree from an array
   *
   * @param arr The input array to build the tree from
   * @param parallel Whether to build the tree in parallel
   * @param granularity The minimum size of a subtree to process in parallel
   * @throws std::invalid_argument if the array is empty
   */
  void build(const std::vector<T> &arr) {
    if (arr.empty()) {
      throw std::invalid_argument("Input array cannot be empty");
    }

    if (arr.size() > n) {
      throw std::invalid_argument("Input array size exceeds segment tree capacity");
    }

    if (parallel) {
#pragma omp parallel
      {
#pragma omp single nowait
        { build_recursive(arr, 0, 0, n - 1); }
      }
    } else {
      build_recursive(arr, 0, 0, n - 1);
    }

    constructed = true;
  }

  /**
   * @brief Query the minimum value in a range
   *
   * @param l Left boundary of the query range
   * @param r Right boundary of the query range
   * @return The minimum value in the range [l, r]
   * @throws std::invalid_argument if the range is invalid
   * @throws std::runtime_error if the tree has not been constructed
   */
  T query(size_t l, size_t r) const {
    if (!constructed) {
      throw std::runtime_error("Segment tree has not been constructed");
    }

    if (l > r || r >= n) {
      throw std::invalid_argument("Invalid query range in query: [" + std::to_string(l) + ", " + std::to_string(r) +
                                  "]");
    }

    return query_recursive(0, 0, n - 1, l, r);
  }

  /**
   * @brief Find the index of the global minimum value in the tree
   *
   * @return The index of the minimum value in the original array
   * @throws std::runtime_error if the tree has not been constructed
   */
  size_t find_min_index() const {
    if (!constructed) {
      throw std::runtime_error("Segment tree has not been constructed");
    }

    // return find_min_index_recursive(0, 0, n - 1);
    size_t node_idx = 0;
    size_t left = 0;
    size_t right = n - 1;

    while (left < right) {
      size_t mid = (left + right) / 2;

      T left_min = tree[lc(node_idx)];
      T right_min = tree[rc(node_idx)];

      if (left_min <= right_min) {
        node_idx = lc(node_idx);
        right = mid;
      } else {
        node_idx = rc(node_idx);
        left = mid + 1;
      }
    }

    return left;
  }

  /**
   * @brief Update a single value in the segment tree
   *
   * @param pos Position to update
   * @param new_val New value to set
   * @throws std::out_of_range if the position is out of bounds
   * @throws std::runtime_error if the tree has not been constructed
   */
  void update(size_t pos, T new_val) {
    if (!constructed) {
      throw std::runtime_error("Segment tree has not been constructed");
    }

    if (pos >= n) {
      throw std::out_of_range("Position out of bounds: " + std::to_string(pos) + " >= " + std::to_string(n));
    }

    update_recursive(0, 0, n - 1, pos, new_val);
  }

  /**
   * @brief Perform the prefix minimum operation
   *
   * Updates the segment tree based on prefix minimum values from arrow sequences.
   *
   * @param pre The prefix value
   * @param arrows The array of arrow sequences
   * @param now The current indices in the arrow sequences
   * @param parallel Whether to perform the operation in parallel
   * @param granularity The minimum size of a subtree to process in parallel
   * @throws std::invalid_argument if the arrow sequences or now indices are invalid
   * @throws std::runtime_error if the tree has not been constructed
   */
  void prefix_min() {
    if (!prefix_mode) {
      throw std::runtime_error("This is not Prefix mode");
    }

    if (!constructed) {
      throw std::runtime_error("Segment tree has not been constructed");
    }

    if (arrows.size() != n) {
      throw std::invalid_argument("Arrow sequences size does not match segment tree size");
    }

    if (now.size() != n) {
      throw std::invalid_argument("Now indices size does not match segment tree size");
    }

    // Validate now indices
    for (size_t i = 0; i < n; ++i) {
      if (now[i] > arrows[i].size()) {
        throw std::invalid_argument("Invalid now index at position " + std::to_string(i));
      }
    }

    if (parallel) {
#pragma omp parallel
      {
#pragma omp single nowait
        { prefix_min_recursive(0, 0, n - 1, infinity); }
      }
    } else {
      prefix_min_recursive(0, 0, n - 1, infinity);
    }
    // std::cout << "Task count: " << task_count.load() << std::endl;
  }

  /**
   * @brief Function to simulate the Read lambda function for LCS prefix minimum operation
   *
   * @param i Index to read from
   * @param arrows The array of arrow sequences
   * @param now The current indices in the arrow sequences
   * @return The value at position i in arrows[i], or infinity if out of bounds
   */
  T read(size_t i) {
    if (!prefix_mode) {
      throw std::runtime_error("This is not Prefix mode");
    }
    if (now[i] >= arrows[i].size()) {
      return std::numeric_limits<T>::max();  // Return infinity
    }
    return arrows[i][now[i]];
  }

  /**
   * @brief Remove an element at a specified position from the segment tree
   *
   * This method doesn't physically remove the element, but sets its value to infinity,
   * effectively removing it from consideration in min queries.
   *
   * @param pos Position of the element to remove
   * @throws std::out_of_range if the position is out of bounds
   * @throws std::runtime_error if the tree has not been constructed
   */
  void remove(size_t pos) {
    if (!constructed) {
      throw std::runtime_error("Segment tree has not been constructed");
    }

    if (pos >= n) {
      throw std::out_of_range("Position out of bounds: " + std::to_string(pos) + " >= " + std::to_string(n));
    }

    // Set the value at this position to infinity, which is effectively a removal
    // for a min segment tree
    update_recursive(0, 0, n - 1, pos, infinity);
  }
};

template class SegmentTree<int>;
template class SegmentTree<float>;
template class SegmentTree<double>;
template class SegmentTree<long>;
template class SegmentTree<std::string>;
template class SegmentTree<std::pair<int, int>>;