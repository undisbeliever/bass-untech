auto Bass::setMacro(const string& name, const string_vector& parameters, uint ip, bool scoped, bool local) -> void {
  if(stackFrame.size() == 0) return;
  auto& macros = stackFrame[local ? stackFrame.size() - 1 : 0].macros;

  string scopedName = {name, ":", parameters.size()};
  if(scope.size()) scopedName = {scope.merge("."), ".", scopedName};

  if(auto macro = macros.find({scopedName})) {
    macro().parameters = parameters;
    macro().ip = ip;
    macro().scoped = scoped;
  } else {
    macros.insert({scopedName, parameters, ip, scoped});
  }
}

auto Bass::findMacro(const string& name, bool local) -> maybe<Macro&> {
  if(stackFrame.size() == 0) return nothing;
  auto& macros = stackFrame[local ? stackFrame.size() - 1 : 0].macros;

  auto s = scope;
  while(true) {
    string scopedName = {s.merge("."), s.size() ? "." : "", name};
    if(auto macro = macros.find({scopedName})) {
      return macro();
    }
    if(!s) break;
    s.removeRight();
  }

  return nothing;
}

auto Bass::findMacro(const string& name) -> maybe<Macro&> {
  if(auto macro = findMacro(name, true)) return macro();
  if(auto macro = findMacro(name, false)) return macro();
  return nothing;
}

auto Bass::setDefine(const string& name, const string& value, bool local) -> void {
  if(stackFrame.size() == 0) return;
  auto& defines = stackFrame[local ? stackFrame.size() - 1 : 0].defines;

  string scopedName = name;
  if(scope.size()) scopedName = {scope.merge("."), ".", name};

  if(auto define = defines.find({scopedName})) {
    define().value = value;
  } else {
    defines.insert({scopedName, value});
  }
}

auto Bass::findDefine(const string& name, bool local) -> maybe<Define&> {
  if(stackFrame.size() == 0) return nothing;
  auto& defines = stackFrame[local ? stackFrame.size() - 1 : 0].defines;

  auto s = scope;
  while(true) {
    string scopedName = {s.merge("."), s.size() ? "." : "", name};
    if(auto define = defines.find({scopedName})) {
      return define();
    }
    if(!s) break;
    s.removeRight();
  }

  return nothing;
}

auto Bass::findDefine(const string& name) -> maybe<Define&> {
  if(auto define = findDefine(name, true)) return define();
  if(auto define = findDefine(name, false)) return define();
  return nothing;
}

auto Bass::setVariable(const string& name, int64_t value, bool local) -> void {
  if(stackFrame.size() == 0) return;
  auto& variables = stackFrame[local ? stackFrame.size() - 1 : 0].variables;

  string scopedName = name;
  if(scope.size()) scopedName = {scope.merge("."), ".", name};

  if(auto variable = variables.find({scopedName})) {
    variable().value = value;
  } else {
    variables.insert({scopedName, value});
  }
}

auto Bass::findVariable(const string& name, bool local) -> maybe<Variable&> {
  if(stackFrame.size() == 0) return nothing;
  auto& variables = stackFrame[local ? stackFrame.size() - 1 : 0].variables;

  auto s = scope;
  while(true) {
    string scopedName = {s.merge("."), s.size() ? "." : "", name};
    if(auto variable = variables.find({scopedName})) {
      return variable();
    }
    if(!s) break;
    s.removeRight();
  }

  return nothing;
}

auto Bass::findVariable(const string& name) -> maybe<Variable&> {
  if(auto variable = findVariable(name, true)) return variable();
  if(auto variable = findVariable(name, false)) return variable();
  return nothing;
}

auto Bass::setConstant(const string& name, int64_t value) -> void {
  string scopedName = name;
  if(scope.size()) scopedName = {scope.merge("."), ".", name};

  if(auto constant = constants.find({scopedName})) {
    if(queryPhase()) error("constant cannot be modified: ", scopedName);
    constant().value = value;
  } else {
    constants.insert({scopedName, value});
  }
}

auto Bass::findConstant(const string& name) -> maybe<Constant&> {
  auto s = scope;
  while(true) {
    string scopedName = {s.merge("."), s.size() ? "." : "", name};
    if(auto constant = constants.find({scopedName})) {
      return constant();
    }
    if(!s) break;
    s.removeRight();
  }

  return nothing;
}

auto Bass::evaluateDefines(string& s) -> void {
  for(int x = s.size() - 1, y = -1; x >= 0; x--) {
    if(s[x] == '}') y = x;
    if(s[x] == '{' && y > x) {
      string name = slice(s, x + 1, y - x - 1);

      if(name.match("defined ?*")) {
        name.trimLeft("defined ", 1L).strip();
        s = {slice(s, 0, x), findDefine(name) ? 1 : 0, slice(s, y + 1)};
        return evaluateDefines(s);
      }

      if(auto define = findDefine(name)) {
        s = {slice(s, 0, x), define().value, slice(s, y + 1)};
        return evaluateDefines(s);
      }
    }
  }
}

auto Bass::filepath() -> string {
  return pathname(sourceFilenames[activeInstruction->fileNumber]);
}

auto Bass::text(string s) -> string {
  if(!s.match("\"*\"")) warning("string value is unqouted: ", s);
  s.trim("\"", "\"", 1L);
  s.replace("\\s", "\'");
  s.replace("\\d", "\"");
  s.replace("\\b", ";");
  s.replace("\\n", "\n");
  s.replace("\\\\", "\\");
  return s;
}

auto Bass::character(string s) -> int64_t {
  if(s[0] != '\'') goto unknown;
  if(s[2] == '\'') return s[1];
  if(s[3] != '\'') goto unknown;
  if(s[1] != '\\') goto unknown;
  if(s[2] == 's') return '\'';
  if(s[2] == 'd') return '\"';
  if(s[2] == 'b') return ';';
  if(s[2] == 'n') return '\n';
  if(s[2] == '\\') return '\\';

unknown:
  warning("unrecognized character constant: ", s);
  return 0;
}
