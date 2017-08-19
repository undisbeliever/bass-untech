auto Bass::execute() -> bool {
  frames.reset();
  conditionals.reset();
  ip = 0;
  macroInvocationCounter = 0;

  initialize();

  frames.append({0, false});
  for(auto& define : defines) {
    setDefine(define.name, define.value, true);
  }

  while(ip < program.size()) {
    Instruction& i = program(ip++);
    if(!executeInstruction(i)) error("unrecognized directive: ", i.statement);
  }

  frames.removeRight();
  return true;
}

auto Bass::executeInstruction(Instruction& i) -> bool {
  activeInstruction = &i;
  string s = i.statement;
  evaluateDefines(s);

  if(s.match("macro ?*(*) {") || s.match("global macro ?*(*) {")) {
    bool inlined = false;
    bool global = s.beginsWith("global ");
    if(global) s.trimLeft("global ", 1L);
    s.trim("macro ", ") {", 1L);
    auto p = s.split("(", 1L).strip();
    auto a = !p(1) ? string_vector{} : p(1).qsplit(",").strip();
    setMacro(p(0), a, ip, inlined, global);
    ip = i.ip;
    return true;
  }

  if(s.match("inline ?*(*) {") || s.match("global inline ?*(*) {")) {
    bool inlined = true;
    bool global = s.beginsWith("global ");
    if(global) s.trimLeft("global ", 1L);
    s.trim("inline ", ") {", 1L);
    auto p = s.split("(", 1L).strip();
    auto a = !p(1) ? string_vector{} : p(1).qsplit(",").strip();
    setMacro(p(0), a, ip, inlined, global);
    ip = i.ip;
    return true;
  }

  if(s.match("define ?*(*)") || s.match("global define ?*(*)")) {
    bool global = s.beginsWith("global ");
    if(global) s.trimLeft("global ", 1L);
    auto p = s.trim("define ", ")", 1L).split("(", 1L).strip();
    setDefine(p(0), p(1), global);
    return true;
  }

  if(s.match("evaluate ?*(*)") || s.match("global evaluate ?*(*)")) {
    bool global = s.beginsWith("global ");
    if(global) s.trimLeft("global ", 1L);
    auto p = s.trim("evaluate ", ")", 1L).split("(", 1L).strip();
    setDefine(p(0), evaluate(p(1)), global);
    return true;
  }

  if(s.match("variable ?*(*)") || s.match("global variable ?*(*)")) {
    bool global = s.beginsWith("global ");
    if(global) s.trimLeft("global ", 1L);
    auto p = s.trim("variable ", ")", 1L).split("(", 1L).strip();
    setVariable(p(0), evaluate(p(1)), global);
    return true;
  }

  if(s.match("if ?* {")) {
    s.trim("if ", " {", 1L).strip();
    bool match = evaluate(s, Evaluation::Strict);
    conditionals.append(match);
    if(match == false) {
      ip = i.ip;
    }
    return true;
  }

  if(s.match("} else if ?* {")) {
    if(conditionals.right()) {
      ip = i.ip;
    } else {
      s.trim("} else if ", " {", 1L).strip();
      bool match = evaluate(s, Evaluation::Strict);
      conditionals.right() = match;
      if(match == false) {
        ip = i.ip;
      }
    }
    return true;
  }

  if(s.match("} else {")) {
    if(conditionals.right()) {
      ip = i.ip;
    } else {
      conditionals.right() = true;
    }
    return true;
  }

  if(s.match("} endif")) {
    conditionals.removeRight();
    return true;
  }

  if(s.match("while ?* {")) {
    s.trim("while ", " {", 1L).strip();
    bool match = evaluate(s, Evaluation::Strict);
    if(match == false) ip = i.ip;
    return true;
  }

  if(s.match("} endwhile")) {
    ip = i.ip;
    return true;
  }

  if(s.match("?*(*)")) {
    auto p = string{s}.trimRight(")", 1L).split("(", 1L);
    auto a = !p(1) ? string_vector{} : p(1).qsplit(",").strip();
    string name = {p(0), ":", a.size()};  //arity overloading
    if(auto macro = findMacro({name})) {
      struct Parameter {
        enum class Type : uint { Define, Variable } type;
        string name;
        string value;
      };

      vector<Parameter> parameters;
      for(uint n : range(a)) {
        auto p = macro().parameters(n).split(" ", 1L).strip();
        if(p.size() == 1) p.prepend("define");

        if(p(0) == "define") parameters.append({Parameter::Type::Define, p(1), a(n)});
        else if(p(0) == "string") parameters.append({Parameter::Type::Define, p(1), text(a(n))});
        else if(p(0) == "evaluate") parameters.append({Parameter::Type::Define, p(1), evaluate(a(n))});
        else if(p(0) == "variable") parameters.append({Parameter::Type::Variable, p(1), evaluate(a(n))});
        else error("unsupported parameter type: ", p(0));
      }

      frames.append({ip, macro().inlined});
      if(!frames.right().inlined) scope.append(p(0));

      setDefine("#", {"_", macroInvocationCounter++, "_"}, false);
      for(auto& parameter : parameters) {
        if(parameter.type == Parameter::Type::Define) setDefine(parameter.name, parameter.value, false);
        if(parameter.type == Parameter::Type::Variable) setVariable(parameter.name, parameter.value.integer(), false);
      }

      ip = macro().ip;
      return true;
    }
  }

  if(s.match("} endmacro") || s.match("} endinline")) {
    ip = frames.right().ip;
    if(!frames.right().inlined) scope.removeRight();
    frames.removeRight();
    return true;
  }

  if(assemble(s)) {
    return true;
  }

  evaluate(s);
  return true;
}
