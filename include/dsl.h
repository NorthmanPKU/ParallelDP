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



enum class ConstraintType { 
    LESS_THAN, 
    GREATER_THAN, 
    EQUAL, 
    NOT_EQUAL,
    NONE
};
// State space definition

struct Var {
    enum class VarType {
        IND,
        SINGLE_DEP,
        RANGE_DEP
    };

    VarType type;
    Var(VarType t) : type(t) {}

    virtual ~Var() = default;
};

struct SingleDepVar;

struct IndVar: Var {
    int min_value;
    int max_value;
    
    IndVar(int min, int max) 
        : Var(VarType::IND), min_value(min), max_value(max) {}

    SingleDepVar* operator+(int v);
    SingleDepVar* operator-(int v);
};

struct SingleDepVar: Var {
    IndVar* base;
    int offset;

    SingleDepVar(IndVar* base, int offset)
        : Var(VarType::SINGLE_DEP), base(base), offset(offset) {}
};

struct RangeDepVar: Var {
    enum class RangeType {
        LIRV, //left int right value
        LVRI,
        LVRV
    };

    Var* min_var;
    Var* max_var;
    int min_val;
    int max_val;
    RangeType range_type;

    RangeDepVar(Var* min_var, Var* max_var) 
        : Var(VarType::RANGE_DEP), min_var(min_var), max_var(max_var), range_type(RangeType::LVRV) {}

    RangeDepVar(int min_val, Var* max_var)
        : Var(VarType::RANGE_DEP), max_var(max_var), min_val(min_val), range_type(RangeType::LIRV) {}

    RangeDepVar(Var* min_var, int max_val)
        : Var(VarType::RANGE_DEP), min_var(min_var), max_val(max_val), range_type(RangeType::LVRI) {}
};

SingleDepVar* IndVar::operator+(int v) {
    SingleDepVar* res = new SingleDepVar(this, v);
    return res;
}

SingleDepVar* IndVar::operator-(int v) {
    SingleDepVar* res = new SingleDepVar(this, -v);
    return res;
}

SingleDepVar* add(IndVar* v, int c) {
    SingleDepVar* res = new SingleDepVar(v, c);
    return res;
}

SingleDepVar* minus(IndVar* v, int c) {
    SingleDepVar* res = new SingleDepVar(v, -c);
    return res;
}

template<typename T> struct Sequence;

template<typename T> struct Val;
// Constraint definition
template<typename T>
struct Constraint {
    Val<T> val1;
    Val<T> val2;
    ConstraintType type;
    
    Constraint(Val<T> v1, Val<T> v2, ConstraintType t)
        : val1(v1), val2(v2), type(t) {}

    // Constraint()
    //     : val1(nullptr), val2(nullptr), type(ConstraintType::NONE) {}
    Constraint()
        : val1(Val<T>(nullptr, nullptr)), val2(Val<T>(nullptr, nullptr)), type(ConstraintType::NONE) {}
};

template<typename T>
struct Val {
    Sequence<T>* seq;
    Var* idx;

    Val(Sequence<T>* s, Var* i)
        : seq(s), idx(i) {}
    
    Constraint<T> operator<(Val<T> v) {
        return Constraint<T>(*this, v, ConstraintType::LESS_THAN);
    }

    Constraint<T> operator>(Val<T> v) {
        return Constraint<T>(*this, v, ConstraintType::GREATER_THAN);
    }

    Constraint<T> operator==(Val<T> v) {
        return Constraint<T>(*this, v, ConstraintType::EQUAL);
    }

    Constraint<T> operator!=(Val<T> v) {
        return Constraint<T>(*this, v, ConstraintType::NOT_EQUAL);
    }
};

template<typename T>
struct Sequence {
    std::vector<T> data;

    Sequence(const std::vector<T>& d) : data(d) {}

    Val<T> operator[](Var* v) {
        return Val<T>(this, v);
    }

    Val<T> operator[](SingleDepVar* v) {

        return Val<T>(this, dynamic_cast<Var*>(v));
    }
};

template<typename T>
Val<T> index(Sequence<T>* seq, SingleDepVar* v) {
    return Val<T>(seq, dynamic_cast<Var*>(v));
}

enum class ExpressionType {
    MAX,
    MIN,
    NUMBER,
    STATUS,
    NONE
};

struct Expression {
    ExpressionType type;

    Expression(ExpressionType t) : type(t) {}
    Expression() : type(ExpressionType::NONE) {}
};

struct TwoPartExpression : Expression {
    Expression left;
    Expression right;

    TwoPartExpression(ExpressionType t, Expression l, Expression r) : Expression(t), left(l), right(r) {}
};

struct Number : TwoPartExpression {
    int value;

    Number(int v) : TwoPartExpression(ExpressionType::NUMBER, Expression(), Expression()), value(v) {}
};

struct Status : Expression {
    // with unknown number of Vars
    std::vector<Var*> vars;
    int dim = 0;

    int constant = 0;

    Status(Var* v1) : Expression(ExpressionType::STATUS), vars({v1}), dim(1) {}
    Status(Var* v1, Var* v2) : Expression(ExpressionType::STATUS), vars({v1, v2}), dim(2) {}

    Status operator+(int c) const {
        Status res = *this;
        res.constant += c;
        return res;
    }

    Status operator-(int c) const {
        Status res = *this;
        res.constant -= c;
        return res;
    }
};

TwoPartExpression max(Expression s1, Expression s2) {
    TwoPartExpression res(ExpressionType::MAX, s1, s2);
    return res;
}

// Data input representation
// Base class for defining DP problems
template<typename U>
class DPProblem {
public:
    virtual ~DPProblem() = default;

    // Problem definition methods
    void addVar(Var* v) {
        if (v->type == Var::VarType::IND) {
            status_dim++;
            state_variables.emplace_back(dynamic_cast<IndVar*>(v));
            std::cout << "Independent variable added" << std::endl;
        } else if (v->type == Var::VarType::SINGLE_DEP) {
            // status_dim++;
            // state_variables.emplace_back(v);
            std::cout << "Single dependency variable added" << std::endl;
        } else if (v->type == Var::VarType::RANGE_DEP) {
            // status_dim++;
            // state_variables.emplace_back(v);
            std::cout << "Range dependency variable added" << std::endl;
        }
    }
    
    template<typename T>
    void addCondition(Constraint<T> c, Expression s) {
        conditions.emplace_back(c, s);
    }

    void addCondition(Expression s) {
        conditions.emplace_back(s);
    }

    void setObjective(Objective obj) {
        objective = obj;
    }

    void setRecurrence(std::function<void(const std::map<std::string, int>&)> recurrence_func) {
        this->recurrence_func = recurrence_func;
    }

    // Data input methods
    template<typename T>
    void addSequence(Sequence<T>* seq) {
        sequences.emplace_back(seq);
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
        if (state_variables.size() == 1) {
            return ProblemType::LIS;
        }
        if (state_variables.size() == 2) {
            return ProblemType::LCS;
        }
        // if (state_variables.size() == 1 && objective == Objective::MAXIMIZE) {
        //     for (const auto& constraint : constraints) {
        //         if (constraint.type == Constraint::ConstraintType::LESS_THAN &&
        //             (constraint.var1 == "j" && constraint.var2 == "i") ||
        //             (constraint.var1 == "prev" && constraint.var2 == "curr")) {
        //             return ProblemType::LIS;
        //         }
        //     }
        // }

        // // LCS recognition pattern
        // if (state_variables.size() == 2 && objective == Objective::MAXIMIZE) {
        //     if (hasSequence("seq1") && hasSequence("seq2")) {
        //         return ProblemType::LCS;
        //     }
        // }

        // // Convex GLWS recognition pattern
        // if (state_variables.size() <= 2 && objective == Objective::MINIMIZE) {
        //     return ProblemType::CONVEX_GLWS;
        // }

        return ProblemType::UNKNOWN;
    }

protected:
    int status_dim = 0;
    std::vector<IndVar*> state_variables;
    std::vector<Sequence<U>*> sequences;
    // std::vector<Constraint> constraints;
    std::vector<std::pair<Constraint<U>, Expression>> conditions;
    Objective objective = Objective::MAXIMIZE;
    std::function<void(const std::map<std::string, int>&)> recurrence_func;
    using DataType = std::variant<std::vector<int>, std::vector<long double>, std::vector<std::string>, long double, int, std::string>;
    std::map<std::string, DataType> data_map;
};

// Solver dispatcher that routes to appropriate backend
class SolverDispatcher {
public:
    template<typename T>
    static T solve(DPProblem<T>& problem) {
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
    static T solveLIS(DPProblem<T>& problem) {
        const auto& sequence = problem.template getSequence<T>("sequence");
        
        // Create backend solver
        LIS<T> solver;
        return solver.compute(sequence);
    }
    
    template<typename T>
    static T solveLCS(DPProblem<T>& problem) {
        const auto& seq1 = problem.template getSequence<T>("seq1");
        const auto& seq2 = problem.template getSequence<T>("seq2");
        
        // Create backend solver
        LCS<int> solver;
        return solver.compute(seq1, seq2);
    }
    
    template<typename T>
    static T solveConvexGLWS(DPProblem<T>& problem) {
        const auto& sequence = problem.template getSequence<T>("data");
        const auto& buildCost = problem.template getValue<T>("buildCost");

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
    
    ProblemBuilder& withVar(Var* v) {
        problem_.addVar(v);
        return *this;
    }

    ProblemBuilder& withVar(int min, Var max) {
        problem_.addDepVar(min, max);
        return *this;
    }
    
    ProblemBuilder& withCondition(Constraint<T> c, Expression s) {
        problem_.addCondition(c, s);
        return *this;
    }

    ProblemBuilder& withCondition(Expression s) {
        problem_.addCondition(Constraint<T>(), s);
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
    
    ProblemBuilder& withSequence(Sequence<T>* seq) {
        problem_.addSequence(seq);
        return *this;
    }

    ProblemBuilder& withValue(const std::string& name, T value) {
        problem_.addValue(name, value);
        return *this;
    }
    
    DPProblem<T> build() {
        return std::move(problem_);
    }
    
private:
    DPProblem<T> problem_;
};

// Convenience factory methods for common problems
// class CommonProblems {
// public:
//     static DPProblem<int> createLIS(const std::vector<int>& sequence) {
//         return ProblemBuilder<int>::create()
//             .withVar("i", 0, sequence.size())
//             .withConstraint("j", "i", ConstraintType::LESS_THAN)
//             .withObjective(Objective::MAXIMIZE)
//             .withSequence("sequence", sequence)
//             .build();
//     }
    
//     static DPProblem<int> createLCS(const std::vector<int>& seq1, const std::vector<int>& seq2) {
//         return ProblemBuilder<int>::create()
//             .withVar("i", 0, seq1.size())
//             .withVar("j", 0, seq2.size())
//             .withObjective(Objective::MAXIMIZE)
//             .withSequence("seq1", seq1)
//             .withSequence("seq2", seq2)
//             .build();
//     }
    
//     static DPProblem<long double> createConvexGLWS(const std::vector<long double>& data, 
//                                     std::function<double(int, int)> cost_func) {
//         auto problem = ProblemBuilder<long double>::create()
//             .withVar("i", 0, data.size())
//             .withObjective(Objective::MINIMIZE)
//             .withSequence("data", data)
//             .build();
            
//         // Store cost function (would need to be extended in actual implementation)
//         return problem;
//     }
// };

} // namespace dp_dsl