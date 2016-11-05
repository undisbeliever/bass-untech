auto Bass::analyze() -> bool {
  blockStack.reset();
  ip = 0;

  while(ip < program.size()) {
    Instruction& i = program(ip++);
    if(!analyzeInstruction(i)) error("unrecognized directive: ", i.statement);
  }

  return true;
}

auto Bass::analyzeInstruction(Instruction& i) -> bool {
  string s = i.statement;

  if(s.match("}") && !blockStack) error("} without matching {");

  if(s.match("{")) {
    blockStack.append({ip - 1, "block"});
    i.statement = "block {";
    return true;
  }

  if(s.match("}") && blockStack.right().type == "block") {
    blockStack.removeRight();
    i.statement = "} endblock";
    return true;
  }

  if(s.match("scope ?* {") || s.match("scope {")) {
    blockStack.append({ip - 1, "scope"});
    return true;
  }

  if(s.match("}") && blockStack.right().type == "scope") {
    blockStack.removeRight();
    i.statement = "} endscope";
    return true;
  }

  if(s.match("macro ?*(*) {")) {
    blockStack.append({ip - 1, "macro"});
    return true;
  }

  if(s.match("}") && blockStack.right().type == "macro") {
    uint rp = blockStack.right().ip;
    program[rp].ip = ip;
    blockStack.removeRight();
    i.statement = "} endmacro";
    return true;
  }

  if(s.match("?*: {") || s.match("- {") || s.match("+ {")) {
    blockStack.append({ip - 1, "constant"});
    return true;
  }

  if(s.match("}") && blockStack.right().type == "constant") {
    blockStack.removeRight();
    i.statement = "} endconstant";
    return true;
  }

  if(s.match("if ?* {")) {
    s.trim("if ", " {", 1L);
    blockStack.append({ip - 1, "if"});
    return true;
  }

  if(s.match("} else if ?* {")) {
    s.trim("} else if ", " {", 1L);
    uint rp = blockStack.right().ip;
    program[rp].ip = ip - 1;
    blockStack.right().ip = ip - 1;
    return true;
  }

  if(s.match("} else {")) {
    uint rp = blockStack.right().ip;
    program[rp].ip = ip - 1;
    blockStack.right().ip = ip - 1;
    return true;
  }

  if(s.match("}") && blockStack.right().type == "if") {
    uint rp = blockStack.right().ip;
    program[rp].ip = ip - 1;
    blockStack.removeRight();
    i.statement = "} endif";
    return true;
  }

  if(s.match("while ?* {")) {
    s.trim("while ", " {", 1L);
    blockStack.append({ip - 1, "while"});
    return true;
  }

  if(s.match("}") && blockStack.right().type == "while") {
    uint rp = blockStack.right().ip;
    program[rp].ip = ip;
    blockStack.removeRight();
    i.statement = "} endwhile";
    i.ip = rp;
    return true;
  }

  return true;
}
