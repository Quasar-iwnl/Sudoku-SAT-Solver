#include <iostream>
#include <vector>
#include <map>
#include <fstream>
#include <string>
#include <set>
#include <sstream>

using namespace std;

class Literal {
public:
    int name;
    int assignment;

    Literal(int name) { 
        this->name = name;
        assignment = -1;
    }

    ~Literal() {}

    Literal(int name, int assignment) {
        this->name = name;
        this->assignment = assignment;
    }

    bool operator==(const Literal& y) const {
        return (this->name == y.name);
    }

    bool operator<(const Literal& other) const {
        return this->name < other.name;
    }

};

class Clause{
public:
    vector<Literal> literals;
    int truth_value;

    Clause() { 
        truth_value = -1;
    }

    Clause(vector<Literal>& literals) { 
        for(int i=0; i<literals.size(); i++) { 
            this->literals.push_back(literals[i]);
        }
        truth_value = -1;
    }

    ~Clause() {}

    void remove(Literal L) { 
        std::vector<Literal>::iterator it = literals.begin();
        while (it != literals.end()) { 
            if (*it == L) { 
                it = literals.erase(it);  // Erase and update iterator
            } else {
                ++it;  // Increment iterator only if not erasing
            }
        }
    }

    void check(Literal L) { 
        // When L is set to true, checks if clause is true, else removes that clause from that literal
        if(truth_value == 1) return;
        for(auto it = literals.begin(); it != literals.end(); it++) { 
            if(it->name == L.name) { 
                truth_value = 1;
                break;
            }
            else if(it->name == -L.name) {
                literals.erase(it);
            }
        }
    }

};

class DPLL {
public:
    vector<Clause> F;
    set<Literal> vars;
    int true_clauses;
    bool sat;

    DPLL(vector<Clause>& F, set<Literal>& vars) { 
        for(int i=0; i<F.size(); i++) { 
            this->F.push_back(F[i]);
        }
        for(auto var : vars) { 
            this->vars.insert(var);
        }
        sat = -1;
        true_clauses = 0;
    }

    DPLL(const DPLL& other) {
        this->F = other.F;         
        this->vars = other.vars;   
        this->sat = other.sat;     
        this->true_clauses = other.true_clauses;
    }

    bool unit_propagate() { 
        for(auto clause : F) { 
            if(clause.truth_value != 1 && clause.literals.size() == 1) { 
                if(!clause.literals[0].assignment) {
                    sat = 0;
                    return false;
                }
                clause.literals[0].assignment = 1;
                clause.truth_value = 1;
                true_clauses++;     
                cout << true_clauses << endl;
                for(auto check_clause : F) { 
                    check_clause.check(clause.literals[0]);
                }
            }
        }
        return true;
    }


    Literal* choose_unassigned() { 
        for(auto clause : F) { 
            if(clause.truth_value != 1 && clause.literals.size() > 0) { 
                return &clause.literals[0];
            }
        }
        return nullptr;
    }

    void pure_elim() { 
        map<int, int> mp;
        vector<Literal> pure;
        for(auto clause : F) {
            if(clause.truth_value != 1) {
                for(auto var : clause.literals) {
                    if(mp[abs(var.name)] == 0) { 
                        mp[abs(var.name)] = (var.name > 0) ? 1 : 0;
                    }
                    else if(mp[abs(var.name)] == 1) { 
                        if(var.name < 0) {
                            mp[abs(var.name)] = -10; // Indicates impure
                        }
                    }
                    else if(mp[abs(var.name)] == -1) { 
                        if(var.name > 0) { 
                            mp[abs(var.name)] = -10; // Impure
                        }
                    }
                }
            }
        }
        for(auto var : vars) { 
            if(mp[var.name] != -10) {
                pure.push_back(var); // Pure variable to be eliminated
                var.assignment = (var.name > 0) ? 1 : 0; //assignment of pure variable
            }
        }

        //Checking each clause for the pure variable and eliminating it
        for(auto var : pure) { 
            for(auto clause : F) { 
                if(clause.truth_value != 1) {
                    for(auto literal : clause.literals) { 
                        if(literal.name == var.name) { 
                            clause.truth_value = 1;
                            true_clauses++;
                        }
                    }
                }
            }
        }
        return;
    }


    bool run() { 
        bool check = unit_propagate();   
        cout << "X" << endl;
        if(!check) return false;
        if(true_clauses == F.size()) return true;
        pure_elim();
        cout << "Y" << endl;
        if(true_clauses == F.size()) return true;
        Literal* l = choose_unassigned();     
        if(l == nullptr) return true;
        l->assignment = true;
        cout << "Z" << endl;
        DPLL decide_true(this->F, this->vars);
        bool left = decide_true.run();
        cout << "W" << endl;
        l->assignment = false;
        DPLL decide_false(this->F, this->vars);
        bool right = decide_false.run();
        if(sat == -1) sat = left || right;
        return sat;
    }

};


int main() {
    std::ifstream file("input.cnf");  // Open the .cnf file
    if (!file.is_open()) {
        std::cerr << "Error: Could not open the file." << std::endl;
        return 1;
    }
    
    set<Literal> literals;
    vector<Clause> Clauses;
    std::string line;
    int num_vars = 0, num_clauses = 0;
    // std::vector<std::vector<int>> clauses;

    while (std::getline(file, line)) {
        // Ignore comment lines
        if (line[0] == 'c') continue;

        // Read the problem line
        if (line[0] == 'p') {
            std::istringstream iss(line);
            std::string tmp;
            iss >> tmp >> tmp >> num_vars >> num_clauses;
            std::cout << "Number of variables: " << num_vars << std::endl;
            std::cout << "Number of clauses: " << num_clauses << std::endl;
        }
        // Read the clauses
        else {
            std::istringstream iss(line);
            int literal;
            vector<Literal> current; 
            while (iss >> literal) {
                if(literal == 0) { 
                    Clauses.push_back(current);
                    current.clear();
                }
                Literal temp(literal);
                literals.insert(temp);
                current.push_back(temp);
            }
        }
    }

    file.close();

    DPLL dp(Clauses, literals);

    // Output the clauses
    // std::cout << "Clauses:" << std::endl;
    // for (const auto& clause : Clauses) {
    //     for (Literal literal : clause.literals) {
    //         std::cout << literal.name << " ";
    //     }
    //     std::cout << std::endl;
    // }

    // dp.pure_elim();
    // dp.pure_elim();
    // cout << dp.sat << endl;
    cout << dp.run() << endl; 

    return 0;
}