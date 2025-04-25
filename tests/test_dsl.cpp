#include "dsl.h"
#include <iostream>
#include <vector>

int main() {
    using namespace dp_dsl;
    
    // Example 1: Defining LIS problem using builder pattern
    auto lis_problem = ProblemBuilder<int>::create()
        .withStateVariable("i", 0, 10)
        .withConstraint("j", "i", Constraint::ConstraintType::LESS_THAN)
        .withObjective(Objective::MAXIMIZE)
        .withSequence("sequence", {3, 1, 4, 2, 7, 5, 8, 6, 9, 10})
        .withRecurrence([](const std::map<std::string, int>& state) {
            // Recurrence relation: dp[i] = max(dp[j] + 1) for all j < i where seq[j] < seq[i]
            // This would be interpreted by the backend
        })
        .build();
    
    // Let the system recognize the problem type
    ProblemType type = lis_problem.getProblemType();
    std::cout << "Detected problem type: " << 
        (type == ProblemType::LIS ? "LIS" : 
         type == ProblemType::LCS ? "LCS" : 
         type == ProblemType::CONVEX_GLWS ? "Convex GLWS" : "Unknown") 
        << std::endl;
    
    // Solve the problem
    int lis_result = SolverDispatcher::solve<int>(lis_problem);
    std::cout << "LIS result: " << lis_result << std::endl;
    
    // Example 2: Using convenience factory for LCS
    std::vector<int> seq1 = {1, 2, 3, 4, 5};
    std::vector<int> seq2 = {3, 1, 4, 2, 5};
    
    auto lcs_problem = CommonProblems::createLCS(seq1, seq2);
    int lcs_result = SolverDispatcher::solve<int>(lcs_problem);
    std::cout << "LCS result: " << lcs_result << std::endl;
    
    // Example 3: Manual problem definition for Convex GLWS
    auto glws_problem = ProblemBuilder<long double>::create()
        .withStateVariable("i", 0, 20)
        .withStateVariable("j", 0, 20)
        .withObjective(Objective::MINIMIZE)
        .withConstraint("i", "j", Constraint::ConstraintType::LESS_THAN)
        .withSequence("data", std::vector<long double>{1.0, 2.5, 3.0, 4.2, 5.1, 6.3})
        .withRecurrence([](const std::map<std::string, int>& state) {
            // Recurrence relation for convex GLWS
            // dp[i][j] = min(dp[i][k] + cost(k, j)) for all k in [i, j)
        })
        .build();
    
    double glws_result = SolverDispatcher::solve<long double>(glws_problem);
    std::cout << "Convex GLWS result: " << glws_result << std::endl;
    
    // Example 4: Generic problem definition that will be analyzed
    auto generic_problem = ProblemBuilder<int>::create()
        .withStateVariable("position", 0, 100)
        .withObjective(Objective::MAXIMIZE)
        .withSequence("values", {10, 20, 30, 15, 25, 35})
        .build();
    
    // The system will analyze the structure and determine what kind of problem this is
    ProblemType generic_type = generic_problem.getProblemType();
    std::cout << "Generic problem detected as: " << 
        (generic_type == ProblemType::LIS ? "LIS" : 
         generic_type == ProblemType::LCS ? "LCS" : 
         generic_type == ProblemType::CONVEX_GLWS ? "Convex GLWS" : "Unknown") 
        << std::endl;
    
    return 0;
}