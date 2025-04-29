#include "dsl.h"
#include <iostream>
#include <vector>

using namespace dp_dsl;

void detectProblemType(DPProblem<int> &problem) {
    ProblemType type = problem.getProblemType();
    std::cout << "Detected problem type: " << 
        (type == ProblemType::LIS ? "LIS" : 
         type == ProblemType::LCS ? "LCS" : 
         type == ProblemType::CONVEX_GLWS ? "Convex GLWS" : "Unknown") 
        << std::endl;
}

int main() {
    
    // Example 1: Defining LIS problem using builder pattern
    std::cout << "Example 1: LIS" << std::endl;
    auto Seq = new Sequence<int>({3, 1, 4, 2, 7, 5, 8, 6, 9, 10});
    auto I = new IndVar(0, 10);
    auto J = new RangeDepVar(0, I-1);
    auto problem1 = ProblemBuilder<int>::create()
        .withVar(I)
        .withVar(J)
        .withSequence(Seq)
        .withCondition(dp_dsl::max(Status(J) + 1, Status(I)))
        .build();
    // Let the system recognize the problem type
    detectProblemType(problem1);
    auto ans = problem1.solve();
    std::cout << "Answer: " << ans << std::endl;

    // Example 2: Defining LCS problem using builder pattern
    std::cout << "--------------------------------" << std::endl;
    std::cout << "Example 2: LCS" << std::endl;
    auto Seq1 = new Sequence<int>({1, 2, 3, 4, 5});
    auto Seq2 = new Sequence<int>({3, 1, 4, 2, 5});
    auto I2 = new IndVar(0, 5);
    auto J2 = new IndVar(0, 5);

    auto problem2 = ProblemBuilder<int>::create()
        .withVar(I2)
        .withVar(J2)
        .withSequence(Seq1)
        .withSequence(Seq2)
        .withCondition(index(Seq1, minus(I2, 1)) == index(Seq2, minus(J2, 1)), Status(I2, J2) + 1)
        .withCondition(index(Seq1, minus(I2, 1)) != index(Seq2, minus(J2, 1)), 
                    dp_dsl::max(Status(I2, minus(J2, 1)), Status(minus(I2, 1), J2)))
        .build();
    
    // Let the system recognize the problem type
    detectProblemType(problem2);
    auto ans2 = problem2.solve();
    std::cout << "Answer: " << ans2 << std::endl;
    std::cout << "--------------------------------" << std::endl;
    return 0;
}