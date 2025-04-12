#include "../../include/segment_tree.h"
#include <iostream>

void example() {
    const size_t n = 8;
    SegmentTree<int> seg_tree(n);

    // Create some test data
    std::vector<int> data = {9, 5, 2, 7, 3, 8, 4, 6};
    
    // Build the segment tree
    seg_tree.build(data);
    
    // Query the minimum value in a range
    std::cout << "Minimum in range [1, 5]: " << seg_tree.query(1, 5) << std::endl;
    
    // Update a value
    seg_tree.update(2, 10);
    std::cout << "Minimum in range [1, 5] after update: " << seg_tree.query(1, 5) << std::endl;
    
    // Example of arrows and now indices for prefix_min
    std::vector<std::vector<int>> arrows(n);
    std::vector<size_t> now(n, 0);
    
    // Fill arrows with sorted values
    arrows[0] = {1, 3, 5, 7, 9};
    arrows[1] = {2, 4, 6, 8, 10};
    arrows[2] = {3, 6, 9, 12, 15};
    arrows[3] = {4, 8, 12, 16, 20};
    arrows[4] = {5, 10, 15, 20, 25};
    arrows[5] = {6, 12, 18, 24, 30};
    arrows[6] = {7, 14, 21, 28, 35};
    arrows[7] = {8, 16, 24, 32, 40};
    
    // Build the tree using the read function
    std::vector<int> initial_values(n);
    for (size_t i = 0; i < n; ++i) {
        initial_values[i] = SegmentTree<int>::read(i, arrows, now);
    }
    seg_tree.build(initial_values);
    
    // Perform prefix_min operation
    std::cout << "Minimum before prefix_min: " << seg_tree.query(0, n - 1) << std::endl;
    seg_tree.prefix_min(6, arrows, now);
    std::cout << "Minimum after prefix_min: " << seg_tree.query(0, n - 1) << std::endl;
    
    // Print current now indices
    std::cout << "Now indices after prefix_min: ";
    for (size_t i = 0; i < n; ++i) {
        std::cout << now[i] << " ";
    }
    std::cout << std::endl;
}

int main() {
    try {
        example();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}