#pragma once

#include <vector>
#include <iostream>
#include <random>
#include <fstream>
#include <sstream>
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

std::pair<std::vector<int>, std::vector<int>> generateLCS(int length1, int length2, int lcsLength) {
    // 参数验证
    if (lcsLength > std::min(length1, length2)) {
        std::cerr << "Error: LCS length cannot be greater than the minimum of the two array lengths" << std::endl;
        exit(1);
    }

    // Find if data file exists
    std::string filename = "lcs_data_" + std::to_string(length1) + "_" + std::to_string(length2) + "_" + std::to_string(lcsLength) + ".txt";
    std::ifstream file(filename);
    // 使用占位符值初始化序列
    std::vector<int> seq1(length1, -1);
    std::vector<int> seq2(length2, -1);
    if (file.is_open()) {
        std::string line, temp;
        // 读取第一个序列
        std::getline(file, line);
        std::istringstream iss1(line.substr(line.find(":") + 1));
        seq1.clear();
        while (iss1 >> temp) {
            seq1.push_back(std::stoi(temp));
        }
        
        // 读取第二个序列
        std::getline(file, line);
        std::istringstream iss2(line.substr(line.find(":") + 1));
        seq2.clear();
        while (iss2 >> temp) {
            seq2.push_back(std::stoi(temp));
        }
        
        file.close();
        return {seq1, seq2};
    }


    // set random seed
    std::random_device rd;
    std::mt19937 gen(rd());

    // 创建一个拥有不同值的公共子序列
    std::vector<int> lcsValues(lcsLength);
    for (int i = 0; i < lcsLength; i++) {
        lcsValues[i] = (i + 1) * 100;  // 使用间隔较大的值
    }
    
    // 为两个序列中的公共元素创建位置
    // 这些位置将是严格递增的，以保持子序列关系
    std::vector<int> pos1, pos2;
    
    // 为序列1选择lcsLength个位置
    for (int i = 0; i < lcsLength; i++) {
        // 确保位置递增
        int minPos = (i > 0) ? pos1.back() + 1 : 0;
        int maxPos = length1 - (lcsLength - i - 1) - 1;
        std::uniform_int_distribution<int> dist(minPos, maxPos);
        pos1.push_back(dist(gen));
    }
    
    // 为序列2选择lcsLength个位置
    for (int i = 0; i < lcsLength; i++) {
        // 确保位置递增
        int minPos = (i > 0) ? pos2.back() + 1 : 0;
        int maxPos = length2 - (lcsLength - i - 1) - 1;
        std::uniform_int_distribution<int> dist(minPos, maxPos);
        pos2.push_back(dist(gen));
    }
    
    
    // 在选定的位置放置公共元素
    for (int i = 0; i < lcsLength; i++) {
        seq1[pos1[i]] = lcsValues[i];
        seq2[pos2[i]] = lcsValues[i];
    }
    
    // 用唯一值填充剩余位置
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

    // Save results
    std::ofstream outFile("lcs_data_" + std::to_string(length1) + "_" + std::to_string(length2) + "_" + std::to_string(lcsLength) + ".txt");
    if (outFile.is_open()) {
        outFile << "Sequence 1: ";
        for (int i = 0; i < length1; i++) {
            outFile << seq1[i] << " ";
        }
        outFile << "\nSequence 2: ";
        for (int i = 0; i < length2; i++) {
            outFile << seq2[i] << " ";
        }
        outFile.close();
    }
    
    return {seq1, seq2};
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
int lcs_dp_naive(const std::vector<T>& seq1, const std::vector<T>& seq2) {
    int m = seq1.size();
    int n = seq2.size();
    
    if (m < n) {
        return lcs_dp_naive(seq2, seq1);
    }
    
    std::vector<int> dp(n + 1, 0);
    
    for (int i = 1; i <= m; i++) {
        int prev = 0;  // 存储dp[i-1][j-1]的值
        for (int j = 1; j <= n; j++) {
            int temp = dp[j];  // 暂存当前的dp[j]，它将成为下一轮的dp[i-1][j-1]
            if (seq1[i-1] == seq2[j-1]) {
                dp[j] = prev + 1;
            } else {
                dp[j] = std::max(dp[j], dp[j-1]);
            }
            prev = temp;
        }
    }
    
    return dp[n];
}