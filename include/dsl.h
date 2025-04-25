#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <variant>
#include <stdexcept>
#include "utils.h"
#include "lis.h"
#include "lcs.h"
#include "glws.h"

namespace dp_dsl {


// Enumeration for different DP problem types
enum class ProblemType {
    LIS,
    LCS,
    CONVEX_GLWS,
    UNKNOWN
};

// Enumeration for optimization objective
enum class Objective {
    MAXIMIZE,
    MINIMIZE
};

// State space definition
struct StateVariable {
    std::string name;
    int min_value;
    int max_value;
    
    StateVariable(const std::string& n, int min, int max) 
        : name(n), min_value(min), max_value(max) {}
};

// Constraint definition
struct Constraint {
    enum class ConstraintType { LESS_THAN, GREATER_THAN, EQUAL, NOT_EQUAL };
    
    std::string var1;
    std::string var2;
    ConstraintType type;
    
    Constraint(const std::string& v1, const std::string& v2, ConstraintType t)
        : var1(v1), var2(v2), type(t) {}
};

// Data input representation
// Base class for defining DP problems
class DPProblem {
public:
    virtual ~DPProblem() = default;

    // Problem definition methods
    void addStateVariable(const std::string& name, int min, int max) {
        state_variables.emplace_back(name, min, max);
    }

    void addConstraint(const std::string& var1, const std::string& var2, 
                      Constraint::ConstraintType type) {
        constraints.emplace_back(var1, var2, type);
    }

    void setObjective(Objective obj) {
        objective = obj;
    }

    void setRecurrence(std::function<void(const std::map<std::string, int>&)> recurrence_func) {
        this->recurrence_func = recurrence_func;
    }

    // Data input methods
    template<typename T>
    void addSequence(const std::string& name, const std::vector<T>& data) {
        data_map[name] = data;
    }

    template<typename T>
    void addValue(const std::string& name, T value) {
        data_map[name] = value;
    }

    template<typename T>
    const std::vector<T>& getSequence(const std::string& name) const {
        auto it = data_map.find(name);
        if (it == data_map.end()) {
            throw std::runtime_error("Sequence not found: " + name);
        }
        return std::get<std::vector<T>>(it->second);
    }

    template<typename T>
    const T getValue(const std::string& name) const {
        auto it = data_map.find(name);
        if (it == data_map.end()) {
            throw std::runtime_error("Value not found: " + name);
        }
        return std::get<T>(it->second);
    }

    bool hasSequence(const std::string& name) const {
        return data_map.find(name) != data_map.end();
    }

    // Problem recognition
    ProblemType getProblemType() const {
        // LIS recognition pattern
        if (state_variables.size() == 1 && objective == Objective::MAXIMIZE) {
            for (const auto& constraint : constraints) {
                if (constraint.type == Constraint::ConstraintType::LESS_THAN &&
                    (constraint.var1 == "j" && constraint.var2 == "i") ||
                    (constraint.var1 == "prev" && constraint.var2 == "curr")) {
                    return ProblemType::LIS;
                }
            }
        }

        // LCS recognition pattern
        if (state_variables.size() == 2 && objective == Objective::MAXIMIZE) {
            if (hasSequence("seq1") && hasSequence("seq2")) {
                return ProblemType::LCS;
            }
        }

        // Convex GLWS recognition pattern
        if (state_variables.size() <= 2 && objective == Objective::MINIMIZE) {
            return ProblemType::CONVEX_GLWS;
        }

        return ProblemType::UNKNOWN;
    }

protected:
    std::vector<StateVariable> state_variables;
    std::vector<Constraint> constraints;
    Objective objective = Objective::MAXIMIZE;
    std::function<void(const std::map<std::string, int>&)> recurrence_func;
    using DataType = std::variant<std::vector<int>, std::vector<long double>, std::vector<std::string>, long double, int, std::string>;
    std::map<std::string, DataType> data_map;
};

// Solver dispatcher that routes to appropriate backend
class SolverDispatcher {
public:
    template<typename T>
    static T solve(DPProblem& problem) {
        ProblemType type = problem.getProblemType();
        
        switch (type) {
            case ProblemType::LIS:
                return solveLIS<T>(problem);
            case ProblemType::LCS:
                return solveLCS<T>(problem);
            case ProblemType::CONVEX_GLWS:
                return solveConvexGLWS<T>(problem);
            default:
                throw std::runtime_error("Cannot solve problem: unknown type");
        }
    }
    
private:
    template<typename T>
    static T solveLIS(DPProblem& problem) {
        const auto& sequence = problem.getSequence<int>("sequence");
        
        // Create backend solver
        LIS<int> solver;
        return solver.compute(sequence);
    }
    
    template<typename T>
    static T solveLCS(DPProblem& problem) {
        const auto& seq1 = problem.getSequence<int>("seq1");
        const auto& seq2 = problem.getSequence<int>("seq2");
        
        // Create backend solver
        LCS<int> solver;
        return solver.compute(seq1, seq2);
    }
    
    template<typename T>
    static T solveConvexGLWS(DPProblem& problem) {
        const auto& sequence = problem.getSequence<T>("data");
        const auto& buildCost = problem.getValue<T>("buildCost");

        auto costFunc = [&](int j, int i, const std::vector<T>& pos) -> T {  // [j+1, i]
            if (i - j < 1) return buildCost;
            int len = i - j;
            int mid_idx = j + 1 + (len - 1) / 2;
            T median = pos[mid_idx];
            T cost = 0;
            for (int k = j + 1; k <= i; ++k) {
            cost += std::abs(pos[k] - median);
            }
            return cost + buildCost;
        };
        
        // Create backend solver  
        ConvexGLWS<T> solver;
        return solver.compute(sequence, costFunc, std::less<T>());
    }
};

// Builder pattern for easier problem creation
template<typename T>
class ProblemBuilder {
public:
    static ProblemBuilder create() {
        return ProblemBuilder();
    }
    
    ProblemBuilder& withStateVariable(const std::string& name, int min, int max) {
        problem_.addStateVariable(name, min, max);
        return *this;
    }
    
    ProblemBuilder& withConstraint(const std::string& var1, const std::string& var2, 
                                 Constraint::ConstraintType type) {
        problem_.addConstraint(var1, var2, type);
        return *this;
    }
    
    ProblemBuilder& withObjective(Objective obj) {
        problem_.setObjective(obj);
        return *this;
    }
    
    ProblemBuilder& withRecurrence(std::function<void(const std::map<std::string, int>&)> func) {
        problem_.setRecurrence(func);
        return *this;
    }
    
    ProblemBuilder& withSequence(const std::string& name, const std::vector<T>& data) {
        problem_.addSequence(name, data);
        return *this;
    }

    ProblemBuilder& withValue(const std::string& name, T value) {
        problem_.addValue(name, value);
        return *this;
    }
    
    DPProblem build() {
        return std::move(problem_);
    }
    
private:
    DPProblem problem_;
};

// Convenience factory methods for common problems
class CommonProblems {
public:
    static DPProblem createLIS(const std::vector<int>& sequence) {
        return ProblemBuilder<int>::create()
            .withStateVariable("i", 0, sequence.size())
            .withConstraint("j", "i", Constraint::ConstraintType::LESS_THAN)
            .withObjective(Objective::MAXIMIZE)
            .withSequence("sequence", sequence)
            .build();
    }
    
    static DPProblem createLCS(const std::vector<int>& seq1, const std::vector<int>& seq2) {
        return ProblemBuilder<int>::create()
            .withStateVariable("i", 0, seq1.size())
            .withStateVariable("j", 0, seq2.size())
            .withObjective(Objective::MAXIMIZE)
            .withSequence("seq1", seq1)
            .withSequence("seq2", seq2)
            .build();
    }
    
    static DPProblem createConvexGLWS(const std::vector<long double>& data, 
                                    std::function<double(int, int)> cost_func) {
        auto problem = ProblemBuilder<long double>::create()
            .withStateVariable("i", 0, data.size())
            .withObjective(Objective::MINIMIZE)
            .withSequence("data", data)
            .build();
            
        // Store cost function (would need to be extended in actual implementation)
        return problem;
    }
};

} // namespace dp_dsl