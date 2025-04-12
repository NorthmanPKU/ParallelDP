#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <thread>
#include <atomic>
#include <iomanip>
#include <omp.h>
#include <algorithm>
#include <functional>
#include <fstream>
#include <sstream>

#include "../../include/tournament_tree.h"

class TournamentTreeBenchmark {
private:
    struct BenchmarkConfig {
        int tree_capacity;        // Capacity of the tree (leaf nodes)
        int num_threads;          // Number of threads
        int num_operations;       // Operations per thread
        double insert_ratio;      // Ratio of insert operations
        double extract_ratio;     // Ratio of extract operations
        double replace_ratio;     // Ratio of replace operations
        double query_ratio;       // Ratio of query operations
        bool use_openmp;          // Whether to use OpenMP
        
        BenchmarkConfig(int cap, int threads, int ops,
                        double i_ratio, double e_ratio, double r_ratio, double q_ratio,
                        bool omp = false)
            : tree_capacity(cap), num_threads(threads), num_operations(ops),
              insert_ratio(i_ratio), extract_ratio(e_ratio), replace_ratio(r_ratio),
              query_ratio(q_ratio), use_openmp(omp) {}
    };
    
    struct BenchmarkResult {
        BenchmarkConfig config;
        double elapsed_time_ms;
        double operations_per_second;
        std::vector<int> operations_per_thread;
        
        BenchmarkResult(const BenchmarkConfig& cfg, double time, double ops,
                        const std::vector<int>& ops_per_thread)
            : config(cfg), elapsed_time_ms(time), operations_per_second(ops),
              operations_per_thread(ops_per_thread) {}
    };
    
    std::vector<BenchmarkResult> results;

public:
    // Run all benchmarks
    void run_benchmarks() {
        std::cout << "Starting Lock-Free Tournament Tree benchmarks..." << std::endl;
        
        // Various configurations to test
        run_single_thread_benchmark();
        run_thread_scaling_benchmark();
        run_operation_mix_benchmarks();
        run_capacity_scaling_benchmark();
        run_openmp_benchmark();
        
        print_summary_report();
        export_csv_report("tournament_tree_benchmark_results.csv");
    }

private:
    // Helper functions
    template <typename T>
    void initialize_tree(LockFreeTournamentTree<T>& tree, int initial_count) {
        // Fill the tree with initial values
        for (int i = 0; i < initial_count && i < tree.capacity(); ++i) {
            tree.insert(i, static_cast<T>(1000 + i));
        }
    }
    
    // Benchmark execution function for std::thread
    template <typename T>
    void run_benchmark_with_threads(const BenchmarkConfig& config) {
        LockFreeTournamentTree<T> tree(config.tree_capacity);
        
        // Initialize tree with data
        initialize_tree(tree, config.tree_capacity / 2);
        
        std::vector<std::thread> threads;
        std::vector<int> operations_per_thread(config.num_threads, 0);
        std::atomic<bool> start_flag(false);
        
        // Prepare random generators for each thread
        std::vector<std::mt19937> random_generators;
        for (int i = 0; i < config.num_threads; ++i) {
            random_generators.emplace_back(42 + i); // Deterministic seeds
        }
        
        // Create and configure operation distributions
        std::discrete_distribution<int> op_dist({
            config.insert_ratio,
            config.extract_ratio,
            config.replace_ratio,
            config.query_ratio
        });
        
        // Launch threads
        for (int t = 0; t < config.num_threads; ++t) {
            threads.emplace_back([&, t]() {
                auto& gen = random_generators[t];
                std::uniform_int_distribution<int> index_dist(0, config.tree_capacity - 1);
                std::uniform_int_distribution<int> value_dist(1, 10000);
                
                // Wait for start signal
                while (!start_flag.load(std::memory_order_relaxed)) {
                    std::this_thread::yield();
                }
                
                // Perform operations
                for (int i = 0; i < config.num_operations; ++i) {
                    int op = op_dist(gen);
                    
                    switch (op) {
                        case 0: // Insert
                            tree.insert(index_dist(gen), value_dist(gen));
                            break;
                            
                        case 1: // Extract
                            tree.extract_winner();
                            break;
                            
                        case 2: // Replace
                            tree.replace_winner(value_dist(gen));
                            break;
                            
                        case 3: // Query
                            tree.get_winner();
                            break;
                    }
                    
                    operations_per_thread[t]++;
                }
            });
        }
        
        // Start timing
        auto start_time = std::chrono::high_resolution_clock::now();
        start_flag.store(true, std::memory_order_relaxed);
        
        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }
        
        // Stop timing
        auto end_time = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // Calculate operations per second
        int total_ops = std::accumulate(operations_per_thread.begin(), 
                                        operations_per_thread.end(), 0);
        double ops_per_second = static_cast<double>(total_ops) / 
                               (elapsed.count() / 1000.0);
        
        // Record results
        results.emplace_back(config, elapsed.count(), ops_per_second, operations_per_thread);
    }
    
    template <typename T>
    void run_benchmark_with_openmp(const BenchmarkConfig& config) {
        LockFreeTournamentTree<T> tree(config.tree_capacity);
        
        initialize_tree(tree, config.tree_capacity / 2);
        
        std::vector<int> operations_per_thread(config.num_threads, 0);
        
        // Configure operation distributions
        std::vector<std::discrete_distribution<int>> op_dists;
        for (int i = 0; i < config.num_threads; ++i) {
            op_dists.emplace_back(std::initializer_list<double>{
                config.insert_ratio,
                config.extract_ratio,
                config.replace_ratio,
                config.query_ratio
            });
        }
        
        // Start timing
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Execute operations using OpenMP
        #pragma omp parallel num_threads(config.num_threads)
        {
            int thread_id = omp_get_thread_num();
            std::mt19937 gen(42 + thread_id);
            std::uniform_int_distribution<int> index_dist(0, config.tree_capacity - 1);
            std::uniform_int_distribution<int> value_dist(1, 10000);
            
            for (int i = 0; i < config.num_operations; ++i) {
                int op = op_dists[thread_id](gen);
                
                switch (op) {
                    case 0: // Insert
                        tree.insert(index_dist(gen), value_dist(gen));
                        break;
                        
                    case 1: // Extract
                        tree.extract_winner();
                        break;
                        
                    case 2: // Replace
                        tree.replace_winner(value_dist(gen));
                        break;
                        
                    case 3: // Query
                        tree.get_winner();
                        break;
                }
                
                operations_per_thread[thread_id]++;
            }
        }
        
        // Stop timing
        auto end_time = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // Calculate operations per second
        int total_ops = std::accumulate(operations_per_thread.begin(), 
                                        operations_per_thread.end(), 0);
        double ops_per_second = static_cast<double>(total_ops) / 
                               (elapsed.count() / 1000.0);
        
        // Record results
        results.emplace_back(config, elapsed.count(), ops_per_second, operations_per_thread);
    }
    
    // Run different benchmark scenarios
    void run_single_thread_benchmark() {
        std::cout << "Running single-thread benchmark..." << std::endl;
        
        // Equal operation mix (25% each)
        BenchmarkConfig config(1024, 1, 1000000, 0.25, 0.25, 0.25, 0.25);
        run_benchmark_with_threads<int>(config);
    }
    
    void run_thread_scaling_benchmark() {
        std::cout << "Running thread scaling benchmark..." << std::endl;
        
        // Test with different thread counts: 1, 2, 4, 8, 16, 32
        for (int threads : {1, 2, 4, 8, 16, 32}) {
            std::cout << "  Testing with " << threads << " threads..." << std::endl;
            
            // Balanced operation mix
            BenchmarkConfig config(1024, threads, 500000 / threads, 0.25, 0.25, 0.25, 0.25);
            run_benchmark_with_threads<int>(config);
        }
    }
    
    void run_operation_mix_benchmarks() {
        std::cout << "Running operation mix benchmarks..." << std::endl;
        
        // Read-heavy (80% get_winner)
        {
            std::cout << "  Testing read-heavy workload..." << std::endl;
            BenchmarkConfig config(1024, 8, 100000, 0.05, 0.05, 0.10, 0.80);
            run_benchmark_with_threads<int>(config);
        }
        
        // Write-heavy (80% insert/extract/replace)
        {
            std::cout << "  Testing write-heavy workload..." << std::endl;
            BenchmarkConfig config(1024, 8, 100000, 0.40, 0.20, 0.20, 0.20);
            run_benchmark_with_threads<int>(config);
        }
        
        // Balanced (equal operations)
        {
            std::cout << "  Testing balanced workload..." << std::endl;
            BenchmarkConfig config(1024, 8, 100000, 0.25, 0.25, 0.25, 0.25);
            run_benchmark_with_threads<int>(config);
        }
    }
    
    void run_capacity_scaling_benchmark() {
        std::cout << "Running capacity scaling benchmark..." << std::endl;
        
        // Test with different tree capacities
        for (int capacity : {16, 64, 256, 1024, 4096, 16384}) {
            std::cout << "  Testing with capacity " << capacity << "..." << std::endl;
            
            // Balanced operation mix
            BenchmarkConfig config(capacity, 8, 100000, 0.25, 0.25, 0.25, 0.25);
            run_benchmark_with_threads<int>(config);
        }
    }
    
    void run_openmp_benchmark() {
        std::cout << "Running OpenMP benchmark..." << std::endl;
        
        // Compare standard threads vs OpenMP with same configuration
        {
            std::cout << "  Testing with standard threads..." << std::endl;
            BenchmarkConfig config_std(1024, 8, 100000, 0.25, 0.25, 0.25, 0.25, false);
            run_benchmark_with_threads<int>(config_std);
        }
        
        {
            std::cout << "  Testing with OpenMP..." << std::endl;
            BenchmarkConfig config_omp(1024, 8, 100000, 0.25, 0.25, 0.25, 0.25, true);
            run_benchmark_with_openmp<int>(config_omp);
        }
    }
    
    // Report generation
    void print_summary_report() {
        std::cout << "\n====== Tournament Tree Benchmark Report ======\n" << std::endl;
        std::cout << std::left << std::setw(8) << "Threads" 
                  << std::setw(10) << "Capacity" 
                  << std::setw(15) << "Operations" 
                  << std::setw(12) << "Time (ms)" 
                  << std::setw(18) << "Ops/sec" 
                  << std::setw(20) << "Operation Mix (IERQ)" 
                  << std::setw(10) << "OpenMP" << std::endl;
        
        std::cout << std::string(90, '-') << std::endl;
        
        for (const auto& result : results) {
            const auto& config = result.config;
            
            // Format operation mix
            std::ostringstream op_mix;
            op_mix << std::fixed << std::setprecision(0) 
                   << config.insert_ratio * 100 << "/"
                   << config.extract_ratio * 100 << "/"
                   << config.replace_ratio * 100 << "/"
                   << config.query_ratio * 100;
            
            std::cout << std::left 
                      << std::setw(8) << config.num_threads
                      << std::setw(10) << config.tree_capacity
                      << std::setw(15) << (config.num_operations * config.num_threads)
                      << std::setw(12) << std::fixed << std::setprecision(2) << result.elapsed_time_ms
                      << std::setw(18) << std::fixed << std::setprecision(0) << result.operations_per_second
                      << std::setw(20) << op_mix.str()
                      << std::setw(10) << (config.use_openmp ? "Yes" : "No")
                      << std::endl;
        }
        
        std::cout << std::endl;
    }
    
    void export_csv_report(const std::string& filename) {
        std::ofstream csv_file(filename);
        
        if (!csv_file.is_open()) {
            std::cerr << "Error: Could not open CSV file for writing." << std::endl;
            return;
        }
        
        // Write CSV header
        csv_file << "Threads,Capacity,Operations,Time_ms,Ops_per_second,"
                 << "Insert_Ratio,Extract_Ratio,Replace_Ratio,Query_Ratio,Using_OpenMP\n";
        
        // Write results
        for (const auto& result : results) {
            const auto& config = result.config;
            
            csv_file << config.num_threads << ","
                     << config.tree_capacity << ","
                     << (config.num_operations * config.num_threads) << ","
                     << std::fixed << std::setprecision(2) << result.elapsed_time_ms << ","
                     << std::fixed << std::setprecision(2) << result.operations_per_second << ","
                     << std::fixed << std::setprecision(4) << config.insert_ratio << ","
                     << std::fixed << std::setprecision(4) << config.extract_ratio << ","
                     << std::fixed << std::setprecision(4) << config.replace_ratio << ","
                     << std::fixed << std::setprecision(4) << config.query_ratio << ","
                     << (config.use_openmp ? "1" : "0") << "\n";
        }
        
        csv_file.close();
        std::cout << "Results exported to " << filename << std::endl;
    }
};

int main() {
    TournamentTreeBenchmark benchmark;
    benchmark.run_benchmarks();
    return 0;
}