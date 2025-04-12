#include <parallel_dp/ParallelDP.hpp>
using parallel_dp::dsl::DPProblem;
int main() {
    std::string A = "...";  // assume sequences are given
    std::string B = "...";
    int n = A.size();
    int m = B.size();
    DPProblem<int, int, int> lcs("LCS");
    lcs.setDomain("i", 0, n);
    lcs.setDomain("j", 0, m);
    // Base cases: if either string is empty, LCS length = 0.
    for (int j = 0; j <= m; ++j) lcs.setBaseCase({0, j}, 0);
    for (int i = 0; i <= n; ++i) lcs.setBaseCase({i, 0}, 0);
    // Recurrence relation
    lcs.setRecurrence([&](int i, int j, DPProblem<int,int,int>& self) -> int {
        if (i == 0 || j == 0) {
            return 0; // boundary (already covered by base cases)
        }
        if (A[i-1] == B[j-1]) {
            return self(i-1, j-1) + 1;
        } else {
            int skipA = self(i-1, j);   // LCS excluding A[i]
            int skipB = self(i, j-1);   // LCS excluding B[j]
            return std::max(skipA, skipB);
        }
    });
    // Solve using parallel DAG scheduler (by default). 
    int lcs_length = lcs.solveParallel();
    std::cout << "LCS length = " << lcs_length << std::endl;
}
