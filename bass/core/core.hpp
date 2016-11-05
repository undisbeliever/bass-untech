struct Bass {
  auto target(const string& filename, bool create) -> bool;
  auto source(const string& filename) -> bool;
  auto define(const string& name, const string& value) -> void;
  auto constant(const string& name, const string& value) -> void;
  auto assemble(bool strict = false) -> bool;

protected:
  enum class Phase : uint { Analyze, Query, Write };
  enum class Endian : uint { LSB, MSB };
  enum class Evaluation : uint { Default = 0, Strict = 1 };  //strict mode disallows forward-declaration of constants

  struct Instruction {
    string statement;
    uint ip;

    uint fileNumber;
    uint lineNumber;
    uint blockNumber;
  };

  struct Macro {
    Macro() {}
    Macro(const string& name) : name(name) {}
    Macro(const string& name, const string_vector& parameters, uint ip, bool scoped) : name(name), parameters(parameters), ip(ip), scoped(scoped) {}

    auto hash() const -> uint { return name.hash(); }
    auto operator==(const Macro& source) const -> bool { return name == source.name; }
    auto operator< (const Macro& source) const -> bool { return name <  source.name; }

    string name;
    string_vector parameters;
    uint ip;
    bool scoped;
  };

  struct Define {
    Define() {}
    Define(const string& name) : name(name) {}
    Define(const string& name, const string& value) : name(name), value(value) {}

    auto hash() const -> uint { return name.hash(); }
    auto operator==(const Define& source) const -> bool { return name == source.name; }
    auto operator< (const Define& source) const -> bool { return name <  source.name; }

    string name;
    string value;
  };

  struct Variable {
    Variable() {}
    Variable(const string& name) : name(name) {}
    Variable(const string& name, int64_t value) : name(name), value(value) {}

    auto hash() const -> uint { return name.hash(); }
    auto operator==(const Variable& source) const -> bool { return name == source.name; }
    auto operator< (const Variable& source) const -> bool { return name <  source.name; }

    string name;
    int64_t value;
  };

  using Constant = Variable;  //Variable and Constant structures are identical

  struct StackFrame {
    uint ip;
    bool scoped;

    hashset<Macro> macros;
    hashset<Define> defines;
    hashset<Variable> variables;
  };

  struct BlockStack {
    uint ip;
    string type;
  };

  auto analyzePhase() const -> bool { return phase == Phase::Analyze; }
  auto queryPhase() const -> bool { return phase == Phase::Query; }
  auto writePhase() const -> bool { return phase == Phase::Write; }

  //core.cpp
  auto pc() const -> uint;
  auto seek(uint offset) -> void;
  auto write(uint64_t data, uint length = 1) -> void;

  auto printInstruction() -> void;
  template<typename... P> auto notice(P&&... p) -> void;
  template<typename... P> auto warning(P&&... p) -> void;
  template<typename... P> auto error(P&&... p) -> void;

  //evaluate.cpp
  auto evaluate(const string& expression, Evaluation mode = Evaluation::Default) -> int64_t;
  auto evaluate(Eval::Node* node, Evaluation mode) -> int64_t;
  auto evaluateParameters(Eval::Node* node, Evaluation mode) -> vector<int64_t>;
  auto evaluateFunction(Eval::Node* node, Evaluation mode) -> int64_t;
  auto evaluateLiteral(Eval::Node* node, Evaluation mode) -> int64_t;
  auto evaluateAssign(Eval::Node* node, Evaluation mode) -> int64_t;

  //analyze.cpp
  auto analyze() -> bool;
  auto analyzeInstruction(Instruction& instruction) -> bool;

  //execute.cpp
  auto execute() -> bool;
  auto executeInstruction(Instruction& instruction) -> bool;

  //assemble.cpp
  virtual auto initialize() -> void;
  virtual auto assemble(const string& statement) -> bool;

  //utility.cpp
  auto setMacro(const string& name, const string_vector& parameters, uint ip, bool scoped, bool local) -> void;
  auto findMacro(const string& name, bool local) -> maybe<Macro&>;
  auto findMacro(const string& name) -> maybe<Macro&>;

  auto setDefine(const string& name, const string& value, bool local) -> void;
  auto findDefine(const string& name, bool local) -> maybe<Define&>;
  auto findDefine(const string& name) -> maybe<Define&>;

  auto setVariable(const string& name, int64_t value, bool local) -> void;
  auto findVariable(const string& name, bool local) -> maybe<Variable&>;
  auto findVariable(const string& name) -> maybe<Variable&>;

  auto setConstant(const string& name, int64_t value) -> void;
  auto findConstant(const string& name) -> maybe<Constant&>;

  auto evaluateDefines(string& statement) -> void;

  auto filepath() -> string;
  auto text(string s) -> string;
  auto character(string s) -> int64_t;

  //internal state
  Instruction* activeInstruction = nullptr;  //used by notice, warning, error
  vector<Instruction> program;    //parsed source code statements
  vector<BlockStack> blockStack;  //track the start and end of blocks
  set<Define> defines;            //defines specified on the terminal
  hashset<Constant> constants;    //constants support forward-declaration
  vector<StackFrame> stackFrame;  //macros, defines and variables do not
  vector<bool> ifStack;           //track conditional matching
  string_vector pushStack;        //track push, pull directives
  string_vector scope;            //track scope recursion
  int64_t stringTable[256];       //overrides for d[bwldq] text strings
  Phase phase;                    //phase of assembly
  Endian endian = Endian::LSB;    //used for multi-byte writes (d[bwldq], etc)
  uint macroInvocationCounter;    //used for {#} support
  uint ip = 0;                    //instruction pointer into program
  uint origin = 0;                //file offset
  int base = 0;                   //file offset to memory map displacement
  uint lastLabelCounter = 1;      //- instance counter
  uint nextLabelCounter = 1;      //+ instance counter
  bool strict = false;            //upgrade warnings to errors when true

  file targetFile;
  string_vector sourceFilenames;
};
