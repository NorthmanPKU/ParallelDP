#include <algorithm>
#include <atomic>
#include <cassert>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <type_traits>
#include <vector>

/**
 * @brief Lock-Free Tournament Tree implementation
 *
 * A tournament tree is a binary tree structure where each non-leaf node
 * represents the "winner" (min or max) of a comparison between its two children.
 * This implementation uses atomic operations to provide thread-safe concurrent access
 * without traditional locks.
 *
 * Implementation characteristics:
 * - Wait-free reads for get_winner and is_empty operations
 * - Lock-free writes for insert, extract_winner, and replace_winner operations
 * - ABA protection through versioning
 * - Full memory order constraints for correctness
 *
 * @tparam T The data type stored in the tree (must be trivially copyable for atomic operations)
 * @tparam Compare Comparison function object type (defaults to std::less for min-tournament)
 */
template <typename T, typename Compare = std::less<T>>
class LockFreeTournamentTree {
  static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable for lock-free operations");

 private:
  // Node structure with versioned value for ABA prevention
  struct alignas(128) Node {  // Cacheline alignment to prevent false sharing
    struct VersionedValue {
      T value;
      uint64_t version;

      VersionedValue() : value(T{}), version(0) {}
      VersionedValue(const T &val, uint64_t ver) : value(val), version(ver) {}

      bool operator==(const VersionedValue &other) const { return value == other.value && version == other.version; }
    };

    std::atomic<VersionedValue> versioned_value;
    std::atomic<bool> needs_update;
    size_t index;
    size_t left_child_index;
    size_t right_child_index;
    size_t parent_index;
    bool is_leaf;

    // Constructor for internal nodes
    Node(size_t idx, size_t left_idx, size_t right_idx, size_t parent_idx)
        : needs_update(false),
          index(idx),
          left_child_index(left_idx),
          right_child_index(right_idx),
          parent_index(parent_idx),
          is_leaf(false) {
      versioned_value.store(VersionedValue(), std::memory_order_relaxed);
    }

    // Constructor for leaf nodes
    Node(size_t idx, size_t parent_idx, const T &value)
        : needs_update(false),
          index(idx),
          left_child_index(SIZE_MAX),
          right_child_index(SIZE_MAX),
          parent_index(parent_idx),
          is_leaf(true) {
      versioned_value.store(VersionedValue(value, 0), std::memory_order_relaxed);
    }
  };

  std::vector<std::unique_ptr<Node>> nodes;  // Tree structure

  size_t leaf_count;                     // Number of leaf nodes
  size_t total_nodes;                    // Total number of nodes
  Compare comp;                          // Comparison function
  T sentinel_value;                      // Sentinel value for undefined/invalid states
  std::atomic<uint64_t> global_version;  // Global version counter for ABA protection

 public:
  /**
   * @brief Construct a new Lock-Free Tournament Tree
   *
   * @param capacity Initial capacity for leaf nodes
   * @param sentinel Value to use as sentinel (defaults to max value for min-tournament)
   * @param comparator Custom comparator function object (optional)
   */
  LockFreeTournamentTree(size_t capacity, T sentinel = std::numeric_limits<T>::max(), Compare comparator = Compare())
      : leaf_count(capacity), sentinel_value(sentinel), comp(comparator), global_version(0) {
    // Calculate the size of a complete binary tree
    size_t height = 0;
    while ((1ULL << height) < capacity) {
      height++;
    }

    // Calculate total nodes for a complete binary tree
    total_nodes = (1ULL << (height + 1)) - 1;
    nodes.resize(total_nodes);

    // Initialize leaf nodes with sentinel values
    size_t first_leaf = (1ULL << height) - 1;
    for (size_t i = 0; i < leaf_count; ++i) {
      size_t leaf_idx = first_leaf + i;
      size_t parent_idx = (leaf_idx - 1) / 2;

      if (leaf_idx < total_nodes) {
        nodes[leaf_idx] = std::make_unique<Node>(leaf_idx, parent_idx, sentinel_value);
      }
    }

    // Fill any remaining leaf slots with sentinel values
    for (size_t i = first_leaf + leaf_count; i < total_nodes; ++i) {
      size_t parent_idx = (i - 1) / 2;
      nodes[i] = std::make_unique<Node>(i, parent_idx, sentinel_value);
    }

    // Initialize internal nodes
    for (size_t i = 0; i < first_leaf; ++i) {
      size_t left_idx = 2 * i + 1;
      size_t right_idx = 2 * i + 2;
      size_t parent_idx = (i > 0) ? (i - 1) / 2 : SIZE_MAX;

      nodes[i] = std::make_unique<Node>(i, left_idx, right_idx, parent_idx);
    }

    // Build the initial tournament
    rebuild_tournament();
  }

  /**
   * @brief Insert a value at a specific leaf position
   *
   * @param index The leaf index (0-based, relative to leaves)
   * @param value The value to insert
   * @return true if successful, false otherwise
   */
  bool insert(size_t index, const T &value) {
    if (index >= leaf_count) {
      return false;
    }

    // Calculate the actual index in the nodes array
    size_t first_leaf = total_nodes - leaf_count * 2 + 1;
    size_t leaf_idx = first_leaf + index;

    if (leaf_idx >= total_nodes) {
      return false;
    }

    // Get the leaf node
    Node *leaf_node = nodes[leaf_idx].get();

    // Get current versioned value
    typename Node::VersionedValue current = leaf_node->versioned_value.load(std::memory_order_acquire);

    // Create new versioned value with new version
    uint64_t new_version = global_version.fetch_add(1, std::memory_order_acq_rel);
    typename Node::VersionedValue new_val(value, new_version);

    // Try to update the leaf value using CAS
    bool success = false;
    do {
      success = leaf_node->versioned_value.compare_exchange_strong(current, new_val, std::memory_order_acq_rel,
                                                                   std::memory_order_acquire);

      if (!success) {
        // Update new_val with new version
        new_version = global_version.fetch_add(1, std::memory_order_acq_rel);
        new_val.version = new_version;
      }
    } while (!success);

    // Mark path to root for update
    mark_path_to_root(leaf_idx);

    // Update the tournament path
    update_path_to_root(leaf_idx);

    return true;
  }

  /**
   * @brief Get the current winner (min/max value depending on comparator)
   *
   * @return The current winner value
   */
  T get_winner() const {
    typename Node::VersionedValue root_val = nodes[0]->versioned_value.load(std::memory_order_acquire);
    return root_val.value;
  }

  /**
   * @brief Extract the current winner (min/max value) and replace it with sentinel
   *
   * @return The extracted winner value or sentinel if empty
   */
  T extract_winner() {
    size_t winner_leaf_idx = find_winner_leaf();

    if (winner_leaf_idx >= total_nodes) {
      return sentinel_value;
    }

    Node *winner_leaf = nodes[winner_leaf_idx].get();

    typename Node::VersionedValue current = winner_leaf->versioned_value.load(std::memory_order_acquire);

    // If it's already a sentinel, another thread might have extracted it
    if (current.value == sentinel_value) {
      return sentinel_value;
    }

    // Store the winner value for return
    T winner_value = current.value;

    // Create new versioned value with sentinel and new version
    uint64_t new_version = global_version.fetch_add(1, std::memory_order_acq_rel);
    typename Node::VersionedValue new_val(sentinel_value, new_version);

    // Try to update the leaf value using CAS
    bool success = false;
    do {
      success = winner_leaf->versioned_value.compare_exchange_strong(current, new_val, std::memory_order_acq_rel,
                                                                     std::memory_order_acquire);

      if (!success) {
        // Check if the value changed
        if (current.value != winner_value) {
          // Value changed by another thread, abort extraction
          return sentinel_value;
        }

        // Update new_val with new version
        new_version = global_version.fetch_add(1, std::memory_order_acq_rel);
        new_val.version = new_version;
      }
    } while (!success);

    // Mark path to root for update and update the tournament path
    mark_path_to_root(winner_leaf_idx);
    update_path_to_root(winner_leaf_idx);

    return winner_value;
  }

  /**
   * @brief Replace the winner (min/max value) with a new value
   *
   * @param new_value The new value to replace the winner
   * @return The previous winner value or sentinel if empty
   */
  T replace_winner(const T &new_value) {
    // Find the leaf node that is the current winner
    size_t winner_leaf_idx = find_winner_leaf();

    if (winner_leaf_idx >= total_nodes) {
      return sentinel_value;
    }

    Node *winner_leaf = nodes[winner_leaf_idx].get();

    // Get current versioned value
    typename Node::VersionedValue current = winner_leaf->versioned_value.load(std::memory_order_acquire);

    // If it's a sentinel, tree might be empty
    if (current.value == sentinel_value) {
      return sentinel_value;
    }

    // Store the winner value for return
    T winner_value = current.value;

    // Create new versioned value with new value and new version
    uint64_t new_version = global_version.fetch_add(1, std::memory_order_acq_rel);
    typename Node::VersionedValue new_val(new_value, new_version);

    // Try to update the leaf value using CAS
    bool success = false;
    do {
      success = winner_leaf->versioned_value.compare_exchange_strong(current, new_val, std::memory_order_acq_rel,
                                                                     std::memory_order_acquire);

      if (!success) {
        // Check if the value changed
        if (current.value != winner_value) {
          // Value changed by another thread, abort replacement
          return sentinel_value;
        }

        // Update new_val with new version
        new_version = global_version.fetch_add(1, std::memory_order_acq_rel);
        new_val.version = new_version;
      }
    } while (!success);

    // Mark path to root for update
    mark_path_to_root(winner_leaf_idx);

    // Update the tournament path
    update_path_to_root(winner_leaf_idx);

    return winner_value;
  }

  /**
   * @brief Check if the tree is empty (root has sentinel value)
   *
   * @return true if empty, false otherwise
   */
  bool is_empty() const {
    typename Node::VersionedValue root_val = nodes[0]->versioned_value.load(std::memory_order_acquire);
    return root_val.value == sentinel_value;
  }

  /**
   * @brief Get the current capacity (number of leaf nodes)
   *
   * @return size_t The capacity
   */
  size_t capacity() const { return leaf_count; }

 private:
  /**
   * @brief Rebuild the entire tournament tree
   * Updates all internal nodes based on their children
   */
  void rebuild_tournament() {
    size_t first_internal = total_nodes / 2 - 1;

    for (int i = first_internal; i >= 0; --i) {
      update_node(i);
    }
  }

  /**
   * @brief Mark the path from a node to the root as needing update
   *
   * @param start_idx Index of the node to start from
   */
  void mark_path_to_root(size_t start_idx) {
    size_t current = start_idx;

    // Mark all nodes on the path to root
    while (current > 0) {
      size_t parent = (current - 1) / 2;
      nodes[parent]->needs_update.store(true, std::memory_order_release);
      current = parent;
    }

    // Mark the root
    nodes[0]->needs_update.store(true, std::memory_order_release);
  }

  /**
   * @brief Update a specific node based on its children
   *
   * @param idx Index of the node to update
   * @return true if the node value changed, false otherwise
   */
  bool update_node(size_t idx) {
    if (idx >= total_nodes || nodes[idx]->is_leaf) {
      return false;
    }

    // Get the node and its children
    Node *node = nodes[idx].get();
    size_t left_idx = node->left_child_index;
    size_t right_idx = node->right_child_index;

    // Ensure indices are valid
    if (left_idx >= total_nodes || right_idx >= total_nodes) {
      std::cerr << "Invalid index of children: " << left_idx << " or " << right_idx << std::endl;
      assert(false);
    }

    // Load children values
    typename Node::VersionedValue left_val = nodes[left_idx]->versioned_value.load(std::memory_order_acquire);
    typename Node::VersionedValue right_val = nodes[right_idx]->versioned_value.load(std::memory_order_acquire);

    // Determine the winner value
    T winner;
    if (left_val.value == sentinel_value) {
      winner = right_val.value;
    } else if (right_val.value == sentinel_value) {
      winner = left_val.value;
    } else if (comp(left_val.value, right_val.value)) {
      winner = left_val.value;
    } else {
      winner = right_val.value;
    }

    // Load current node value
    typename Node::VersionedValue current = node->versioned_value.load(std::memory_order_acquire);

    // If the value hasn't changed, no need to update
    if (current.value == winner) {
      node->needs_update.store(false, std::memory_order_release);
      return false;
    }

    // Create new versioned value with winner value and new version
    uint64_t new_version = global_version.fetch_add(1, std::memory_order_acq_rel);
    typename Node::VersionedValue new_val(winner, new_version);

    // Update
    bool success = false;
    do {
      success = node->versioned_value.compare_exchange_strong(current, new_val, std::memory_order_acq_rel,
                                                              std::memory_order_acquire);

      if (!success) {
        if (current.value == winner) {
          node->needs_update.store(false, std::memory_order_release);
          return false;
        }

        // Update new_val with new version
        new_version = global_version.fetch_add(1, std::memory_order_acq_rel);
        new_val.version = new_version;
      }
    } while (!success);

    // Mark as updated
    node->needs_update.store(false, std::memory_order_release);

    return true;
  }

  /**
   * @brief Update the path from a node to the root
   *
   * @param start_idx Index of the node to start from
   */
  void update_path_to_root(size_t start_idx) {
    size_t current = start_idx;

    while (current > 0) {
      size_t parent = (current - 1) / 2;

      // Only update if needed
      if (nodes[parent]->needs_update.load(std::memory_order_acquire)) {
        if (!update_node(parent)) {
          // If the node didn't change, we can stop
          break;
        }
      } else {
        break;
      }

      current = parent;
    }

    // Finally update the root if needed
    if (nodes[0]->needs_update.load(std::memory_order_acquire)) {
      update_node(0);
    }
  }

  /**
   * @brief Find the leaf node that is the current winner
   *
   * @return Index of the winner leaf node or SIZE_MAX if tree is empty
   */
  size_t find_winner_leaf() {
    // Load root value
    typename Node::VersionedValue root_val = nodes[0]->versioned_value.load(std::memory_order_acquire);

    // If tree is empty, return invalid index
    if (root_val.value == sentinel_value) {
      return SIZE_MAX;
    }

    size_t current = 0;
    T winner_value = root_val.value;

    // Traverse down the tree following the winner path
    while (!nodes[current]->is_leaf) {
      size_t left_idx = nodes[current]->left_child_index;
      size_t right_idx = nodes[current]->right_child_index;

      if (left_idx >= total_nodes || right_idx >= total_nodes) {
        std::cerr << "Invalid index of children: " << left_idx << " or " << right_idx << std::endl;
        assert(false);
        break;
      }

      typename Node::VersionedValue left_val = nodes[left_idx]->versioned_value.load(std::memory_order_acquire);
      typename Node::VersionedValue right_val = nodes[right_idx]->versioned_value.load(std::memory_order_acquire);

      // Left first
      if (left_val.value == winner_value) {
        current = left_idx;
      } else if (right_val.value == winner_value) {
        current = right_idx;
      } else {
        // The tree changed during traversal, the winner is no longer valid
        // Start over from the root
        return find_winner_leaf();
      }
    }

    return current;
  }
};