#pragma once

#include "dp_dsl.h"
#include <set>
#include <regex>

namespace dp_dsl {

// Extended problem recognizer with pattern matching
class AdvancedProblemRecognizer {
public:
    struct Pattern {
        int num_state_vars;
        Objective objective;
        std::set<std::string> constraint_patterns;
        std::set<std::string> recurrence_patterns;
        ProblemType type;
        
        Pattern(int vars, Objective obj, 
                const std::set<std::string>& constraints,
                const std::set<std::string>& recurrences,
                ProblemType t)
            : num_state_vars(vars), objective(obj),
              constraint_patterns(constraints),
              recurrence_patterns(recurrences),
              type(t) {}
    };
    
    static void initialize() {
        if (!initialized) {
            registerPatterns();
            initialized = true;
        }
    }
    
    static ProblemType recognize(const DPProblem& problem) {
        initialize();
        
        // Extract problem characteristics
        int num_vars = problem.getStateVariables().size();
        Objective obj = problem.getObjective();
        std::set<std::string> constraint_signatures;
        
        // Analyze constraints
        for (const auto& constraint : problem.getConstraints()) {
            std::string signature = getConstraintSignature(constraint);
            constraint_signatures.insert(signature);
        }
        
        // Match against known patterns
        for (const auto& pattern : patterns) {
            if (pattern.num_state_vars == num_vars &&
                pattern.objective == obj &&
                hasMatchingConstraints(constraint_signatures, pattern.constraint_patterns)) {
                
                return pattern.type;
            }
        }
        
        return ProblemType::UNKNOWN;
    }
    
private:
    static std::vector<Pattern> patterns;
    static bool initialized;
    
    static void registerPatterns() {
        // LIS pattern
        patterns.emplace_back(
            1, // Single state variable
            Objective::MAXIMIZE,
            {"monotonic_increase", "order_constraint"},
            {"max_previous", "subsequence_relation"},
            ProblemType::LIS
        );
        
        // LCS pattern
        patterns.emplace_back(
            2, // Two state variables
            Objective::MAXIMIZE,
            {"sequence_comparison", "2d_constraints"},
            {"diagonal_match", "2d_max"},
            ProblemType::LCS
        );
        
        // Convex GLWS pattern
        patterns.emplace_back(
            1, // Single or double state variable
            Objective::MINIMIZE,
            {"interval_constraint", "convex_property"},
            {"min_cost", "interval_recurrence"},
            ProblemType::CONVEX_GLWS
        );
    }
    
    static std::string getConstraintSignature(const Constraint& constraint) {
        // Convert constraint to a signature string
        if (constraint.type == Constraint::ConstraintType::LESS_THAN) {
            if ((constraint.var1 == "j" && constraint.var2 == "i") ||
                (constraint.var1 == "prev" && constraint.var2 == "curr")) {
                return "monotonic_increase";
            }
        }
        return "generic_constraint";
    }
    
    static bool hasMatchingConstraints(
        const std::set<std::string>& actual,
        const std::set<std::string>& pattern) {
        // Check if actual constraints match any of the pattern constraints
        for (const auto& pat : pattern) {
            if (actual.find(pat) != actual.end()) {
                return true;
            }
        }
        return actual.empty() && pattern.empty();
    }
};

// Extended problem definition with more metadata
class ExtendedDPProblem : public DPProblem {
public:
    // Add metadata for better problem recognition
    void addRecurrenceHint(const std::string& hint) {
        recurrence_hints.push_back(hint);
    }
    
    void addProblemDescription(const std::string& description) {
        problem_description = description;
    }
    
    // Get metadata
    const std::vector<std::string>& getRecurrenceHints() const {
        return recurrence_hints;
    }
    
    const std::string& getProblemDescription() const {
        return problem_description;
    }
    
    // Natural language parser for problem definition
    static ExtendedDPProblem parseFromDescription(const std::string& description) {
        ExtendedDPProblem problem;
        
        // Simple pattern matching for common DP problems
        if (description.find("longest increasing subsequence") != std::string::npos ||
            description.find("LIS") != std::string::npos) {
            setupLISProblem(problem);
        }
        else if (description.find("longest common subsequence") != std::string::npos ||
                 description.find("LCS") != std::string::npos) {
            setupLCSProblem(problem);
        }
        else if (description.find("convex optimization") != std::string::npos ||
                 description.find("GLWS") != std::string::npos) {
            setupConvexGLWSProblem(problem);
        }
        
        problem.addProblemDescription(description);
        return problem;
    }
    
private:
    std::vector<std::string> recurrence_hints;
    std::string problem_description;
    
    static void setupLISProblem(ExtendedDPProblem& problem) {
        problem.addStateVariable("i", 0, 1000);
        problem.addConstraint("j", "i", Constraint::ConstraintType::LESS_THAN);
        problem.setObjective(Objective::MAXIMIZE);
        problem.addRecurrenceHint("max_previous");
        problem.addRecurrenceHint("subsequence_relation");
    }
    
    static void setupLCSProblem(ExtendedDPProblem& problem) {
        problem.addStateVariable("i", 0, 1000);
        problem.addStateVariable("j", 0, 1000);
        problem.setObjective(Objective::MAXIMIZE);
        problem.addRecurrenceHint("diagonal_match");
        problem.addRecurrenceHint("2d_max");
    }
    
    static void setupConvexGLWSProblem(ExtendedDPProblem& problem) {
        problem.addStateVariable("i", 0, 1000);
        problem.addStateVariable("j", 0, 1000);
        problem.addConstraint("i", "j", Constraint::ConstraintType::LESS_THAN);
        problem.setObjective(Objective::MINIMIZE);
        problem.addRecurrenceHint("min_cost");
        problem.addRecurrenceHint("interval_recurrence");
    }
};

// DSL compiler that translates high-level definitions to backend calls
class DSLCompiler {
public:
    struct CompiledProblem {
        ProblemType type;
        std::function<void()> solver_callback;
        std::string ir_code; // Intermediate representation
    };
    
    static CompiledProblem compile(const DPProblem& problem) {
        CompiledProblem compiled;
        compiled.type = problem.getProblemType();
        
        // Generate intermediate representation
        compiled.ir_code = generateIR(problem);
        
        // Create solver callback
        switch (compiled.type) {
            case ProblemType::LIS:
                compiled.solver_callback = createLISSolverCallback(problem);
                break;
            case ProblemType::LCS:
                compiled.solver_callback = createLCSSolverCallback(problem);
                break;
            case ProblemType::CONVEX_GLWS:
                compiled.solver_callback = createGLWSSolverCallback(problem);
                break;
            default:
                throw std::runtime_error("Cannot compile unknown problem type");
        }
        
        return compiled;
    }
    
private:
    static std::string generateIR(const DPProblem& problem) {
        std::stringstream ir;
        ir << "PROBLEM_TYPE: " << static_cast<int>(problem.getProblemType()) << "\n";
        ir << "OBJECTIVE: " << 
            (problem.getObjective() == Objective::MAXIMIZE ? "MAX" : "MIN") << "\n";
        
        ir << "STATE_VARS:\n";
        for (const auto& var : problem.getStateVariables()) {
            ir << "  " << var.name << " [" << var.min_value << ", " << var.max_value << "]\n";
        }
        
        ir << "CONSTRAINTS:\n";
        for (const auto& constraint : problem.getConstraints()) {
            ir << "  " << constraint.var1 << " " << 
                constraintTypeToString(constraint.type) << " " << 
                constraint.var2 << "\n";
        }
        
        return ir.str();
    }
    
    static std::string constraintTypeToString(Constraint::ConstraintType type) {
        switch (type) {
            case Constraint::ConstraintType::LESS_THAN: return "<";
            case Constraint::ConstraintType::GREATER_THAN: return ">";
            case Constraint::ConstraintType::EQUAL: return "==";
            case Constraint::ConstraintType::NOT_EQUAL: return "!=";
            default: return "??";
        }
    }
    
    static std::function<void()> createLISSolverCallback(const DPProblem& problem) {
        return [&problem]() {
            auto& data = const_cast<DPProblem&>(problem).data();
            const auto& sequence = data.getSequence<int>("sequence");
            LIS<int> solver;
            int result = solver.compute(sequence);
            std::cout << "LIS Result: " << result << std::endl;
        };
    }
    
    static std::function<void()> createLCSSolverCallback(const DPProblem& problem) {
        return [&problem]() {
            auto& data = const_cast<DPProblem&>(problem).data();
            const auto& seq1 = data.getSequence<int>("seq1");
            const auto& seq2 = data.getSequence<int>("seq2");
            LCS<int> solver;
            int result = solver.compute(seq1, seq2);
            std::cout << "LCS Result: " << result << std::endl;
        };
    }
    
    static std::function<void()> createGLWSSolverCallback(const DPProblem& problem) {
        return [&problem]() {
            auto& data = const_cast<DPProblem&>(problem).data();
            const auto& sequence = data.getSequence<double>("data");
            auto cost_func = [](int i, int j) -> double { return (j - i) * (j - i); };
            ConvexGLWS<double> solver;
            double result = solver.compute(sequence, cost_func, std::less<double>());
            std::cout << "Convex GLWS Result: " << result << std::endl;
        };
    }
};

// Initialize static members
std::vector<AdvancedProblemRecognizer::Pattern> AdvancedProblemRecognizer::patterns;
bool AdvancedProblemRecognizer::initialized = false;

} // namespace dp_dsl