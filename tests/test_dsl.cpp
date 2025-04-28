#include "dsl.h"
#include <iostream>
#include <vector>

int main() {
    using namespace dp_dsl;
    
    // Example 1: Defining LIS problem using builder pattern
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
    ProblemType type = problem1.getProblemType();
    std::cout << "Detected problem type: " << 
        (type == ProblemType::LIS ? "LIS" : 
         type == ProblemType::LCS ? "LCS" : 
         type == ProblemType::CONVEX_GLWS ? "Convex GLWS" : "Unknown") 
        << std::endl;
    

    // Example 2: Defining LCS problem using builder pattern
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
    ProblemType type2 = problem2.getProblemType();
    std::cout << "Detected problem type: " << 
        (type2 == ProblemType::LIS ? "LIS" : 
         type2 == ProblemType::LCS ? "LCS" : 
         type2 == ProblemType::CONVEX_GLWS ? "Convex GLWS" : "Unknown") 
        << std::endl;

    
    // // Example 3: Manual problem definition for Convex GLWS
    // auto glws_problem = ProblemBuilder<long double>::create()
    //     .withVar("i", 0, 20)
    //     .withVar("j", 0, 20)
    //     .withObjective(Objective::MINIMIZE)
    //     .withConstraint("i", "j", Constraint::ConstraintType::LESS_THAN)
    //     .withSequence("data", std::vector<long double>{1.0, 2.5, 3.0, 4.2, 5.1, 6.3})
    //     .withValue("buildCost", 10.0)
    //     .withRecurrence([](const std::map<std::string, int>& state) {
    //         // Recurrence relation for convex GLWS
    //         // dp[i][j] = min(dp[i][k] + cost(k, j)) for all k in [i, j)
    //     })
    //     .build();
    
    // double glws_result = SolverDispatcher::solve<long double>(glws_problem);
    // std::cout << "Convex GLWS result: " << glws_result << std::endl;
    return 0;
}