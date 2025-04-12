#include <iostream>
#include <vector>
#include <limits>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <string>
#include <omp.h>
#include <memory>
#include <cassert>

template<typename T1, typename T2>
std::ostream& operator<<(std::ostream& os, const std::pair<T1, T2>& p) {
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
    std::vector<T> tree;        // The segment tree array
    size_t n;                   // Number of leaf nodes
    T infinity;                 // Value representing infinity
    bool constructed = false;   // Flag to track if tree has been constructed

    /**
     * @brief Get the index of the left child of a node
     * @param x The index of the parent node
     * @return The index of the left child
     */
    inline size_t lc(size_t x) const {
        return 2 * x + 1;
    }

    /**
     * @brief Get the index of the right child of a node
     * @param x The index of the parent node
     * @return The index of the right child
     */
    inline size_t rc(size_t x) const {
        return 2 * x + 2;
    }

    /**
     * @brief Get the index of the parent node
     * @param x The index of a child node
     * @return The index of the parent node
     */
    inline size_t parent(size_t x) const {
        return (x - 1) / 2;
    }

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
    void build_recursive(const std::vector<T>& arr, size_t x, size_t l, size_t r, 
                        bool parallel, size_t granularity) {
        if (l == r) {
            tree[x] = (l < arr.size()) ? arr[l] : infinity;
            return;
        }

        size_t mid = (l + r) / 2;
        bool do_parallel = parallel && (r - l > granularity);

        if (do_parallel) {
            #pragma omp task shared(arr, tree)
            {
                build_recursive(arr, lc(x), l, mid, parallel, granularity);
            }
            #pragma omp task shared(arr, tree)
            {
                build_recursive(arr, rc(x), mid + 1, r, parallel, granularity);
            }
            #pragma omp taskwait
        } else {
            build_recursive(arr, lc(x), l, mid, parallel, granularity);
            build_recursive(arr, rc(x), mid + 1, r, parallel, granularity);
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
    void prefix_min_recursive(size_t x, size_t l, size_t r, T pre, 
                            const std::vector<std::vector<T>>& arrows, 
                            std::vector<size_t>& now,
                            bool parallel, size_t granularity) {
        // Early return if this node's value is already greater than pre
        if (tree[x] > pre) {
            return;
        }

        // If we've reached a leaf node
        if (l == r) {
            const auto& ys = arrows[l];
            
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
            tree[x] = (now[l] >= ys.size()) ? infinity : ys[now[l]];
            return;
        }

        size_t mid = (l + r) / 2;
        
        // Optimize the traversal based on which child has the minimum value
        if (tree[x] == tree[rc(x)]) {
            if (tree[lc(x)] <= pre && tree[lc(x)] < infinity) {
                bool do_parallel = parallel && (r - l > granularity);
                T lc_val = tree[lc(x)];
                
                if (do_parallel) {
                    #pragma omp task shared(tree, arrows, now)
                    {
                        prefix_min_recursive(lc(x), l, mid, pre, arrows, now, parallel, granularity);
                    }
                    #pragma omp task shared(tree, arrows, now)
                    {
                        prefix_min_recursive(rc(x), mid + 1, r, lc_val, arrows, now, parallel, granularity);
                    }
                    #pragma omp taskwait
                } else {
                    prefix_min_recursive(lc(x), l, mid, pre, arrows, now, parallel, granularity);
                    prefix_min_recursive(rc(x), mid + 1, r, lc_val, arrows, now, parallel, granularity);
                }
            } else {
                prefix_min_recursive(rc(x), mid + 1, r, pre, arrows, now, parallel, granularity);
            }
        } else {
            prefix_min_recursive(lc(x), l, mid, pre, arrows, now, parallel, granularity);
        }
        
        // Update this node's value
        tree[x] = std::min(tree[lc(x)], tree[rc(x)]);
    }

public:
    /**
     * @brief Construct a new Segment Tree object
     * 
     * @param size The number of elements in the original array
     * @param inf_value The value to use as infinity (default: maximum value of type T)
     */
    SegmentTree(size_t size, T inf_value = std::numeric_limits<T>::max())
        : n(size), infinity(inf_value) {
        if (size == 0) {
            throw std::invalid_argument("Segment tree size cannot be zero");
        }
        
        // Allocate memory for the segment tree (4 * n is sufficient)
        tree.resize(4 * n, infinity);
    }

    /**
     * @brief Construct a new Segment Tree object based on an array
     * 
     * @param arr The input array to build the tree from
     * @param inf_value The value to use as infinity (default: maximum value of type T)
     */
    SegmentTree(const std::vector<T>& arr, T inf_value = std::numeric_limits<T>::max(), bool parallel = false, size_t granularity = 1000)
        : n(arr.size()), infinity(inf_value) {
        std::cout << "SegmentTree init" << std::endl;
        std::cout << "arr: " << std::endl;
        for (size_t i = 0; i < arr.size(); ++i) {
            std::cout << arr[i] << " ";
        }
        std::cout << std::endl;
        std::cout << "inf_value: " << inf_value << std::endl;
        std::cout << "parallel: " << parallel << std::endl;
        std::cout << "granularity: " << granularity << std::endl;
        if (arr.empty()) {
            throw std::invalid_argument("Input array cannot be empty");
        }
        tree.resize(4 * n, inf_value);
        build(arr, parallel, granularity);
    }

    /**
     * @brief Get the size of the segment tree
     * 
     * @return The number of leaf nodes in the segment tree
     */
    size_t size() const {
        return n;
    }

    /**
     * @brief Get the value at a specific position in the tree
     * 
     * @param index The position to access
     * @return The value at the given position
     * @throws std::out_of_range if the index is out of bounds
     */
    T at(size_t index) const {
        if (index >= tree.size()) {
            throw std::out_of_range("Index out of bounds: " + std::to_string(index) + 
                                   " >= " + std::to_string(tree.size()));
        }
        return tree[index];
    }

    /**
     * @brief Get the raw tree array
     * 
     * @return A const reference to the tree array
     */
    const std::vector<T>& get_tree() const {
        return tree;
    }

    /**
     * @brief Build the segment tree from an array
     * 
     * @param arr The input array to build the tree from
     * @param parallel Whether to build the tree in parallel
     * @param granularity The minimum size of a subtree to process in parallel
     * @throws std::invalid_argument if the array is empty
     */
    void build(const std::vector<T>& arr, bool parallel = false, size_t granularity = 1000) {
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
                {
                    build_recursive(arr, 0, 0, n - 1, true, granularity);
                }
            }
        } else {
            build_recursive(arr, 0, 0, n - 1, false, granularity);
        }
        
        constructed = true;
    }

    /**
     * @brief Build the segment tree using a read function
     * 
     * @param read_func A function that returns the value at position i
     * @param parallel Whether to build the tree in parallel
     * @param granularity The minimum size of a subtree to process in parallel
     */
    void build_with_function(const std::function<T(size_t)>& read_func, 
                           bool parallel = false, size_t granularity = 1000) {
        std::vector<T> arr(n);
        for (size_t i = 0; i < n; ++i) {
            arr[i] = read_func(i);
        }
        build(arr, parallel, granularity);
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
            throw std::invalid_argument("Invalid query range: [" + std::to_string(l) + 
                                       ", " + std::to_string(r) + "]");
        }
        
        return query_recursive(0, 0, n - 1, l, r);
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
            throw std::out_of_range("Position out of bounds: " + std::to_string(pos) + 
                                   " >= " + std::to_string(n));
        }
        
        update_recursive(0, 0, n - 1, pos, new_val);
    }

    /**
     * @brief Perform the prefix minimum operation as specified in the provided code
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
    void prefix_min(T pre, const std::vector<std::vector<T>>& arrows, 
                   std::vector<size_t>& now, bool parallel = false, size_t granularity = 1000) {
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
                {
                    prefix_min_recursive(0, 0, n - 1, pre, arrows, now, true, granularity);
                }
            }
        } else {
            prefix_min_recursive(0, 0, n - 1, pre, arrows, now, false, granularity);
        }
    }

    /**
     * @brief Function to simulate the Read lambda function from the provided code
     * 
     * @param i Index to read from
     * @param arrows The array of arrow sequences
     * @param now The current indices in the arrow sequences
     * @return The value at position i in arrows[i], or infinity if out of bounds
     */
    static T read(size_t i, const std::vector<std::vector<T>>& arrows, const std::vector<size_t>& now) {
        if (now[i] >= arrows[i].size()) {
            return std::numeric_limits<T>::max(); // Return infinity
        }
        return arrows[i][now[i]];
    }

    /**
     * @brief Helper function to determine whether to execute in parallel
     * 
     * @param parallel Whether parallelism is enabled
     * @param task1 First task to execute
     * @param task2 Second task to execute
     */
    static void conditional_par_do(bool parallel, const std::function<void()>& task1, 
                                  const std::function<void()>& task2) {
        if (parallel) {
            #pragma omp parallel sections
            {
                #pragma omp section
                {
                    task1();
                }
                #pragma omp section
                {
                    task2();
                }
            }
        } else {
            task1();
            task2();
        }
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
            throw std::out_of_range("Position out of bounds: " + std::to_string(pos) + 
                                " >= " + std::to_string(n));
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