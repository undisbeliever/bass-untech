auto Bass::setMacro(const string& name, const string_vector& parameters, uint ip, bool inlined, bool global) -> void {
  if(!validate(name)) error("invalid macro identifier: ", name);
  string scopedName = {scope.merge("."), scope ? "." : "", name, ":", parameters.size()};

  for(int n : rrange(frames.size())) {
    if(global && n) continue;
    if(frames[n].inlined) continue;

    auto& macros = frames[n].macros;
    if(auto macro = macros.find({scopedName})) {
      macro().parameters = parameters;
      macro().ip = ip;
      macro().inlined = inlined;
    } else {
      macros.insert({scopedName, parameters, ip, inlined});
    }

    return;
  }
}

auto Bass::findMacro(const string& name) -> maybe<Macro&> {
  for(int n : rrange(frames.size())) {
    if(frames[n].inlined) continue;

    auto& macros = frames[n].macros;
    auto s = scope;
    while(true) {
      string scopedName = {s.merge("."), s ? "." : "", name};
      if(auto macro = macros.find({scopedName})) {
        return macro();
      }
      if(!s) break;
      s.removeRight();
    }
  }

  return nothing;
}

auto Bass::setDefine(const string& name, const string& value, bool global) -> void {
  if(!validate(name) && name != "#") error("invalid define identifier: ", name);
  string scopedName = {scope.merge("."), scope ? "." : "", name};

  for(int n : rrange(frames.size())) {
    if(global && n) continue;
    if(frames[n].inlined) continue;

    auto& defines = frames[n].defines;
    if(auto define = defines.find({scopedName})) {
      define().value = value;
    } else {
      defines.insert({scopedName, value});
    }

    return;
  }
}

auto Bass::findDefine(const string& name) -> maybe<Define&> {
  for(int n : rrange(frames.size())) {
    if(frames[n].inlined) continue;

    auto& defines = frames[n].defines;
    auto s = scope;
    while(true) {
      string scopedName = {s.merge("."), s ? "." : "", name};
      if(auto define = defines.find({scopedName})) {
        return define();
      }
      if(!s) break;
      s.removeRight();
    }
  }

  return nothing;
}

auto Bass::setVariable(const string& name, int64_t value, bool global) -> void {
  if(!validate(name)) error("invalid variable identifier: ", name);
  string scopedName = {scope.merge("."), scope ? "." : "", name};

  for(int n : rrange(frames.size())) {
    if(global && n) continue;
    if(frames[n].inlined) continue;

    auto& variables = frames[n].variables;
    if(auto variable = variables.find({scopedName})) {
      variable().value = value;
    } else {
      variables.insert({scopedName, value});
    }

    return;
  }
}

auto Bass::findVariable(const string& name) -> maybe<Variable&> {
  for(int n : rrange(frames.size())) {
    if(frames[n].inlined) continue;

    auto& variables = frames[n].variables;
    auto s = scope;
    while(true) {
      string scopedName = {s.merge("."), s ? "." : "", name};
      if(auto variable = variables.find({scopedName})) {
        return variable();
      }
      if(!s) break;
      s.removeRight();
    }
  }

  return nothing;
}

auto Bass::setConstant(const string& name, int64_t value) -> void {
  if(!validate(name)) error("invalid constant identifier: ", name);
  string scopedName = {scope.merge("."), scope ? "." : "", name};

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
    string scopedName = {s.merge("."), s ? "." : "", name};
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
  return Location::path(sourceFilenames[activeInstruction->fileNumber]);
}

//reduce all duplicate whitespace segments (eg "  ") to single whitespace (" ")
auto Bass::strip(string& s) -> void {
  uint offset = 0;
  bool quoted = false;
  for(int n = 0; n < s.size() - 1; n++) {
    if(s[n] == '"') quoted = !quoted;
    if(!quoted && s[n] == ' ' && s[n + 1] == ' ') continue;
    s.get()[offset++] = s[n];
  }
  s.resize(offset);
}

//returns true for valid name identifiers
auto Bass::validate(const string& s) -> bool {
  for(uint n : range(s.size())) {
    char c = s[n];
    if(c == '_') continue;
    if(c == '.' && n) continue;
    if(c >= 'A' && c <= 'Z') continue;
    if(c >= 'a' && c <= 'z') continue;
    if(c >= '0' && c <= '9' && n) continue;
    return false;
  }
  return true;
}

auto Bass::text(string s) -> string {
  if(!s.match("\"*\"")) warning("string value is unquoted: ", s);
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
