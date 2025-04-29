#pragma once

#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <unordered_map>
#include <vector>

// enum including CILK and OpenMP
enum class ParallelArch { CILK, OPENMP, PARLAY, CILK_OPT, NONE };

template <typename T1, typename T2>
std::ostream &operator<<(std::ostream &os, const std::pair<T1, T2> &p) {
  os << "(" << p.first << "," << p.second << ")";
  return os;
}

// Used to store compressed information of the best decisions: indicates that for all states within the interval [l, r],
// their
struct Interval {
  int l;
  int r;
  int j;
};

inline int findBest(int i, const std::vector<Interval> &B) {
  for (const auto &interval : B) {
    if (i >= interval.l && i <= interval.r) return interval.j;
  }
  return 0;  // fallback
}

void get_existing_arrows(int n, int m, int lcs_length, std::vector<std::vector<int>> &arrows) {
  std::string filename =
      "arrow_" + std::to_string(n) + "_" + std::to_string(m) + "_" + std::to_string(lcs_length) + ".txt";
  std::ifstream file(filename);
  if (file.is_open()) {
    arrows.clear();
    arrows.resize(n);
    std::string line;
    int i = 0;
    while (std::getline(file, line) && i < n) {
      std::istringstream iss(line);
      int value;
      while (iss >> value) {
        arrows[i].push_back(value);
      }
      i++;
    }
    std::cout << "Arrows read from " << filename << std::endl;
    return;
  }
  std::cout << "Arrows not found in " << filename << std::endl;
}

template <typename T>
void get_arrows(const std::vector<T> &data1, const std::vector<T> &data2, int lcs_length,
                std::vector<std::vector<int>> &arrows) {
  int n = data1.size();
  int m = data2.size();

  // check if arrow_n_m_lcslen.txt exists
  std::string filename =
      "arrow_" + std::to_string(n) + "_" + std::to_string(m) + "_" + std::to_string(lcs_length) + ".txt";
  std::ifstream file(filename);
  // if exists, read from file
  if (file.is_open()) {
    // read into arrows
    get_existing_arrows(n, m, lcs_length, arrows);
    return;
  }

  arrows = std::vector<std::vector<int>>(n, std::vector<int>(0));

  std::unordered_map<T, std::vector<int>> data2_to_indices;
  for (int j = 0; j < m; j++) {
    data2_to_indices[data2[j]].push_back(j);
  }

  // Get the effective states: (i, j) pairs where data1[i] == data2[j]
  for (int i = 0; i < n; i++) {
    auto it = data2_to_indices.find(data1[i]);
    if (it != data2_to_indices.end()) {
      for (int j : it->second) {
        arrows[i].push_back(j);
      }
    }
  }

  // save to file
  std::ofstream outFile(filename);
  if (outFile.is_open()) {
    for (int i = 0; i < n; i++) {
      for (int j : arrows[i]) {
        outFile << j << " ";
      }
      outFile << "\n";
    }
    outFile.close();
    std::cout << "Arrows saved to " << filename << std::endl;
  }
}

void generateLCS(int length1, int length2, int lcsLength, std::vector<std::vector<int>> &arrows) {
  if (lcsLength > std::min(length1, length2)) {
    std::cerr << "Error: LCS length cannot be greater than the minimum of the two array lengths" << std::endl;
    exit(1);
  }

  // Find if data file exists

  std::string arrow_filename =
      "arrow_" + std::to_string(length1) + "_" + std::to_string(length2) + "_" + std::to_string(lcsLength) + ".txt";
  std::ifstream arrow_file(arrow_filename);
  if (arrow_file.is_open()) {
    std::cout << "Arrows read from " << arrow_filename << std::endl;
    get_existing_arrows(length1, length2, lcsLength, arrows);
    return;
  }

  std::vector<int> seq1(length1, -1);
  std::vector<int> seq2(length2, -1);
  // if (file.is_open()) {
  //     std::string line, temp;
  //     std::getline(file, line);
  //     std::istringstream iss1(line.substr(line.find(":") + 1));
  //     seq1.clear();
  //     while (iss1 >> temp) {
  //         seq1.push_back(std::stoi(temp));
  //     }

  //     std::getline(file, line);
  //     std::istringstream iss2(line.substr(line.find(":") + 1));
  //     seq2.clear();
  //     while (iss2 >> temp) {
  //         seq2.push_back(std::stoi(temp));
  //     }

  //     file.close();
  //     return {seq1, seq2};
  // }

  // set random seed
  std::random_device rd;
  std::mt19937 gen(rd());

  std::vector<int> lcsValues(lcsLength);
  for (int i = 0; i < lcsLength; i++) {
    lcsValues[i] = (i + 1) * 100;
  }

  std::vector<int> pos1, pos2;

  for (int i = 0; i < lcsLength; i++) {
    int minPos = (i > 0) ? pos1.back() + 1 : 0;
    int maxPos = length1 - (lcsLength - i - 1) - 1;
    std::uniform_int_distribution<int> dist(minPos, maxPos);
    pos1.push_back(dist(gen));
  }

  for (int i = 0; i < lcsLength; i++) {
    int minPos = (i > 0) ? pos2.back() + 1 : 0;
    int maxPos = length2 - (lcsLength - i - 1) - 1;
    std::uniform_int_distribution<int> dist(minPos, maxPos);
    pos2.push_back(dist(gen));
  }

  for (int i = 0; i < lcsLength; i++) {
    seq1[pos1[i]] = lcsValues[i];
    seq2[pos2[i]] = lcsValues[i];
  }

  std::uniform_int_distribution<int> uniqueDist1(1, length1);
  std::uniform_int_distribution<int> uniqueDist2(length2, length2 * 2);

  for (int i = 0; i < length1; i++) {
    if (seq1[i] == -1) {
      seq1[i] = uniqueDist1(gen);
    }
  }

  for (int i = 0; i < length2; i++) {
    if (seq2[i] == -1) {
      seq2[i] = uniqueDist2(gen);
    }
  }

  // // Save results
  // std::ofstream outFile("lcs_data_" + std::to_string(length1) + "_" + std::to_string(length2) + "_" +
  // std::to_string(lcsLength) + ".txt"); if (outFile.is_open()) {
  //     outFile << "Sequence 1: ";
  //     for (int i = 0; i < length1; i++) {
  //         outFile << seq1[i] << " ";
  //     }
  //     outFile << "\nSequence 2: ";
  //     for (int i = 0; i < length2; i++) {
  //         outFile << seq2[i] << " ";
  //     }
  //     outFile.close();
  // }

  // return {seq1, seq2};
  get_arrows(seq1, seq2, lcsLength, arrows);
}

std::vector<int> generateLIS(int length, int lisLength) {
  if (lisLength > length) {
    std::cerr << "Error: LIS length cannot be greater than the minimum of the array lengths" << std::endl;
    exit(1);
  }

  // set random seed
  std::random_device rd;
  std::mt19937 gen(rd());

  std::vector<int> lisPositions;
  for (int i = 0; i < lisLength; i++) {
    int minPos = (i == 0) ? 0 : lisPositions.back() + 1;
    int maxPos = length - (lisLength - i);
    std::uniform_int_distribution<int> dist(minPos, maxPos);
    lisPositions.push_back(dist(gen));
  }

  std::vector<int> lisValues(lisLength);
  for (int i = 0; i < lisLength; i++) {
    lisValues[i] = (i + 1) * 100;
  }

  std::vector<int> seq(length, 0);
  if (lisPositions[0] > 0) {
    int low = lisValues[0] + 1;
    int high = lisValues[0] + 50;
    int gapLen = lisPositions[0];
    std::vector<int> gapValues(gapLen);
    for (int i = 0; i < gapLen; i++) {
      std::uniform_int_distribution<int> dist(low, high);
      gapValues[i] = dist(gen);
    }
    std::sort(gapValues.begin(), gapValues.end(), std::greater<int>());
    for (int i = 0; i < gapLen; i++) {
      seq[i] = gapValues[i];
    }
  }

  for (int i = 0; i < lisLength; i++) {
    seq[lisPositions[i]] = lisValues[i];

    int start = lisPositions[i] + 1;
    int end = (i == lisLength - 1) ? length : lisPositions[i + 1];
    if (start < end) {
      int low = 1;
      int high = lisValues[i] - 1;
      int gapLen = end - start;
      std::vector<int> gapValues(gapLen);
      for (int j = 0; j < gapLen; j++) {
        std::uniform_int_distribution<int> dist(low, high);
        gapValues[j] = dist(gen);
      }
      std::sort(gapValues.begin(), gapValues.end(), std::greater<int>());
      for (int j = 0; j < gapLen; j++) {
        seq[start + j] = gapValues[j];
      }
    }
  }
  return seq;
}

template <typename T>
int lcs_dp_naive(const std::vector<T> &seq1, const std::vector<T> &seq2) {
  int m = seq1.size();
  int n = seq2.size();

  if (m < n) {
    return lcs_dp_naive(seq2, seq1);
  }

  std::vector<int> dp(n + 1, 0);

  for (int i = 1; i <= m; i++) {
    int prev = 0;
    for (int j = 1; j <= n; j++) {
      int temp = dp[j];
      if (seq1[i - 1] == seq2[j - 1]) {
        dp[j] = prev + 1;
      } else {
        dp[j] = std::max(dp[j], dp[j - 1]);
      }
      prev = temp;
    }
  }

  return dp[n];
}
