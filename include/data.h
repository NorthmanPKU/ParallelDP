// Copy from https://github.com/ucrparlay/Parallel-Work-Efficient-Dynamic-Programming/blob/main/lcs.cpp
#include <iostream>
#include <random>
#include <omp.h>
//hash function

std::vector<std::vector<int>> MakeRandom(size_t n, unsigned int seed = 0) {
  std::vector<int> a(n);
  std::vector<int> b(n);
  
  std::mt19937 gen(seed);
  std::uniform_int_distribution<int> dist(0, 9);
  
  // Fill the sequences in parallel using OpenMP
  #pragma omp parallel for
  for (size_t i = 0; i < n; i++) {
    if (seed == 0) {
      // Use deterministic hashing if no seed provided (original behavior)
      a[i] = std::hash<size_t>{}(i) % 10;
      b[i] = std::hash<size_t>{}(i + (size_t)(&n)) % 10;
    } else {
      // Use true random number generation with the seed
      unsigned int local_seed = seed + i;
      std::mt19937 local_gen(local_seed);
      a[i] = dist(local_gen);
      b[i] = dist(local_gen);
    }
  }
  
  // Create the arrows structure
  std::vector<std::vector<int>> arrows(n);
  
  #pragma omp parallel for
  for (size_t i = 0; i < n; i++) {
    for (size_t j = 0; j < n; j++) {
      if (a[i] == b[j]) {
        arrows[i].push_back(j);
      }
    }
  }
  
  return arrows;
}

auto MakeData(size_t n, size_t m, size_t k) {
  // std::cout << "MakeData: n: " << n << ", m: " << m << ", k: " << k << std::endl;
  assert(k <= m);
  assert(k <= n);
  assert(m <= k * n * 2 - k * k);
  
  std::vector<std::vector<int>> arrows(n);
  
  #pragma omp parallel for
  for (size_t i = 0; i < k; i++) {
    arrows[i].push_back(i);
  }

  // std::cout << "k arrows pushed" << std::endl;
  
  m -= k;
  
  auto Push = [&](size_t i, const std::vector<size_t>& s) {
    if (m >= s.size()) {
      arrows[i].insert(arrows[i].end(), s.begin(), s.end());
      m -= s.size();
    } else {
      arrows[i].insert(arrows[i].end(), s.begin(), s.begin() + m);
      m = 0;
    }
  };
  
  std::vector<size_t> a(n + 1);
  #pragma omp parallel for
  for (size_t i = 0; i <= n; i++) {
    a[i] = i;
  }

  // std::cout << "n+1 array pushed" << std::endl;
  for (size_t i = 0; i < n; i++) {
    // std::cout << "pushing arrows for i: " << i << std::endl;
    if (m == 0) break;
    if (i < k) {
      std::vector<size_t> slice(a.begin(), a.begin() + i);
      Push(i, slice);
      
      std::sort(arrows[i].begin(), arrows[i].end());
    } else {
      std::vector<size_t> slice(a.begin(), a.begin() + k + 1);
      Push(i, slice);
    }
  }

  // std::cout << "pushing arrows" << std::endl;
  
  for (size_t i = 0; i < n; i++) {
    if (m == 0) break;
    if (i < k) {
      std::vector<size_t> slice(a.begin() + i + 1, a.end());
      Push(i, slice);
    }
  }

  // std::cout << "pushing remaining arrows" << std::endl;
  
  size_t tot = 0;
  #pragma omp parallel for reduction(+:tot)
  for (size_t i = 0; i < n; i++) {
    tot += arrows[i].size();
  }

  return arrows;
}