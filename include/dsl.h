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

// Forward declarations
class DPProblem;
class ProblemRecognizer;
class DataInput;

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
class DataInput {
public:
    using DataType = std::variant<std::vector<int>, std::vector<double>, std::vector<std::string>>;
    
    void addSequence(const std::string& name, const std::vector<int>& data) {
        data_map[name] = data;
    }
    
    void addSequence(const std::string& name, const std::vector<double>& data) {
        data_map[name] = data;
    }
    
    void addSequence(const std::string& name, const std::vector<std::string>& data) {
        data_map[name] = data;
    }
    
    template<typename T>
    const std::vector<T>& getSequence(const std::string& name) const {
        auto it = data_map.find(name);
        if (it == data_map.end()) {
            throw std::runtime_error("Sequence not found: " + name);
        }
        return std::get<std::vector<T>>(it->second);
    }
    
    bool hasSequence(const std::string& name) const {
        return data_map.find(name) != data_map.end();
    }
    
private:
    std::map<std::string, DataType> data_map;
};

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
    
    // Data input
    DataInput& data() {
        return data_input;
    }
    
    // Problem recognition
    ProblemType getProblemType() const {
        return ProblemRecognizer::recognize(*this);
    }
    
    // Getters for problem structure
    const std::vector<StateVariable>& getStateVariables() const { return state_variables; }
    const std::vector<Constraint>& getConstraints() const { return constraints; }
    Objective getObjective() const { return objective; }
    
protected:
    std::vector<StateVariable> state_variables;
    std::vector<Constraint> constraints;
    Objective objective = Objective::MAXIMIZE;
    std::function<void(const std::map<std::string, int>&)> recurrence_func;
    DataInput data_input;
};

// Problem recognizer
class ProblemRecognizer {
public:
    static ProblemType recognize(const DPProblem& problem) {
        // Analyze problem structure to determine type
        const auto& vars = problem.getStateVariables();
        const auto& constraints = problem.getConstraints();
        const auto objective = problem.getObjective();
        
        // LIS recognition pattern
        if (vars.size() == 1 && objective == Objective::MAXIMIZE) {
            // Check for monotonicity constraint
            for (const auto& constraint : constraints) {
                if (constraint.type == Constraint::ConstraintType::LESS_THAN &&
                    (constraint.var1 == "j" && constraint.var2 == "i") ||
                    (constraint.var1 == "prev" && constraint.var2 == "curr")) {
                    return ProblemType::LIS;
                }
            }
        }
        
        // LCS recognition pattern
        if (vars.size() == 2 && objective == Objective::MAXIMIZE) {
            // Check for two sequences
            if (problem.data().hasSequence("seq1") && problem.data().hasSequence("seq2")) {
                return ProblemType::LCS;
            }
        }
        
        // Convex GLWS recognition pattern
        if (vars.size() <= 2 && objective == Objective::MINIMIZE) {
            // Check for cost function signature
            return ProblemType::CONVEX_GLWS;
        }
        
        return ProblemType::UNKNOWN;
    }
};

// Solver dispatcher that routes to appropriate backend
class SolverDispatcher {
public:
    template<typename T>
    static T solve(DPProblem& problem) {
        ProblemType type = problem.getProblemType();
        
        switch (type) {
            case ProblemType::LIS:
                return solveLIS(problem);
            case ProblemType::LCS:
                return solveLCS(problem);
            case ProblemType::CONVEX_GLWS:
                return solveConvexGLWS(problem);
            default:
                throw std::runtime_error("Cannot solve problem: unknown type");
        }
    }
    
private:
    template<typename T>
    static T solveLIS(DPProblem& problem) {
        auto& data = problem.data();
        const auto& sequence = data.getSequence<int>("sequence");
        
        // Create backend solver
        LIS<int> solver;
        return solver.compute(sequence);
    }
    
    template<typename T>
    static T solveLCS(DPProblem& problem) {
        auto& data = problem.data();
        const auto& seq1 = data.getSequence<int>("seq1");
        const auto& seq2 = data.getSequence<int>("seq2");
        
        // Create backend solver
        LCS<int> solver;
        return solver.compute(seq1, seq2);
    }
    
    template<typename T>
    static T solveConvexGLWS(DPProblem& problem) {
        auto& data = problem.data();
        const auto& sequence = data.getSequence<double>("data");
        
        // Extract cost function (would need to be stored in problem definition)
        // For now, use a placeholder
        auto cost_func = [](int i, int j) -> double { return (j - i) * (j - i); };
        
        // Create backend solver  
        ConvexGLWS<double> solver;
        return solver.compute(sequence, cost_func, std::less<double>());
    }
};

// Builder pattern for easier problem creation
class ProblemBuilder {
public:
    static ProblemBuilder create() {
        return ProblemBuilder();
    }
    
    ProblemBuilder& withStateVariable(const std::string& name, int min, int max) {
        problem.addStateVariable(name, min, max);
        return *this;
    }
    
    ProblemBuilder& withConstraint(const std::string& var1, const std::string& var2, 
                                 Constraint::ConstraintType type) {
        problem.addConstraint(var1, var2, type);
        return *this;
    }
    
    ProblemBuilder& withObjective(Objective obj) {
        problem.setObjective(obj);
        return *this;
    }
    
    ProblemBuilder& withRecurrence(std::function<void(const std::map<std::string, int>&)> func) {
        problem.setRecurrence(func);
        return *this;
    }
    
    ProblemBuilder& withSequence(const std::string& name, const std::vector<int>& data) {
        problem.data().addSequence(name, data);
        return *this;
    }
    
    DPProblem build() {
        return std::move(problem);
    }
    
private:
    DPProblem problem;
};

// Convenience factory methods for common problems
class CommonProblems {
public:
    static DPProblem createLIS(const std::vector<int>& sequence) {
        return ProblemBuilder::create()
            .withStateVariable("i", 0, sequence.size())
            .withConstraint("j", "i", Constraint::ConstraintType::LESS_THAN)
            .withObjective(Objective::MAXIMIZE)
            .withSequence("sequence", sequence)
            .build();
    }
    
    static DPProblem createLCS(const std::vector<int>& seq1, const std::vector<int>& seq2) {
        return ProblemBuilder::create()
            .withStateVariable("i", 0, seq1.size())
            .withStateVariable("j", 0, seq2.size())
            .withObjective(Objective::MAXIMIZE)
            .withSequence("seq1", seq1)
            .withSequence("seq2", seq2)
            .build();
    }
    
    static DPProblem createConvexGLWS(const std::vector<double>& data, 
                                    std::function<double(int, int)> cost_func) {
        auto problem = ProblemBuilder::create()
            .withStateVariable("i", 0, data.size())
            .withObjective(Objective::MINIMIZE)
            .withSequence("data", data)
            .build();
            
        // Store cost function (would need to be extended in actual implementation)
        return problem;
    }
};

} // namespace dp_dsl