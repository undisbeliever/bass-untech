auto Bass::initialize() -> void {
  pushStack.reset();
  scope.reset();
  for(uint n : range(256)) stringTable[n] = n;
  endian = Endian::LSB;
  origin = 0;
  base = 0;
  lastLabelCounter = 1;
  nextLabelCounter = 1;
}

auto Bass::assemble(const string& statement) -> bool {
  string s = statement;

  if(s.match("block {")) return true;
  if(s.match("} endblock")) return true;

  //constant name(value)
  if(s.match("constant ?*(*)")) {
    auto p = s.trim("constant ", ")", 1L).split("(", 1L);
    setConstant(p(0), evaluate(p(1)));
    return true;
  }

  //scope name {
  if(s.match("scope ?* {") || s.match("scope {")) {
    s.trim("scope ", "{", 1L).strip();
    if(s.endsWith(":")) setConstant(s.trimRight(":", 1L), pc());
    if(s) validateName(s);
    scope.append(s);
    return true;
  }

  //}
  if(s.match("} endscope")) {
    scope.removeRight();
    return true;
  }

  //label: or label: {
  if(s.match("?*:") || s.match("?*: {")) {
    s.trimRight(" {", 1L);
    s.trimRight(":", 1L);
    setConstant(s, pc());
    return true;
  }

  //- or - {
  if(s.match("-") || s.match("- {")) {
    setConstant({"lastLabel#", lastLabelCounter++}, pc());
    return true;
  }

  //+ or + {
  if(s.match("+") || s.match("+ {")) {
    setConstant({"nextLabel#", nextLabelCounter++}, pc());
    return true;
  }

  //}
  if(s.match("} endconstant")) {
    return true;
  }

  //output "filename" [, create]
  if(s.match("output ?*")) {
    auto p = s.trimLeft("output ", 1L).qsplit(",").strip();
    string filename = {filepath(), p.take(0).trim("\"", "\"", 1L)};
    bool create = (p.size() && p(0) == "create");
    target(filename, create);
    return true;
  }

  //endian (lsb|msb)
  if(s.match("endian ?*")) {
    s.trimLeft("endian ", 1L);
    if(s == "lsb") { endian = Endian::LSB; return true; }
    if(s == "msb") { endian = Endian::MSB; return true; }
    error("invalid endian mode");
  }

  //origin offset
  if(s.match("origin ?*")) {
    s.trimLeft("origin ", 1L);
    origin = evaluate(s);
    seek(origin);
    return true;
  }

  //base offset
  if(s.match("base ?*")) {
    s.trimLeft("base ", 1L);
    base = evaluate(s) - origin;
    return true;
  }

  //push variable [, ...]
  if(s.match("push ?*")) {
    auto p = s.trimLeft("push ", 1L).qsplit(",").strip();
    for(auto& t : p) {
      if(t == "origin") {
        pushStack.append(origin);
      } else if(t == "base") {
        pushStack.append(base);
      } else if(t == "pc") {
        pushStack.append(origin);
        pushStack.append(base);
      } else {
        error("unrecognized push variable: ", t);
      }
    }
    return true;
  }

  //pull variable [, ...]
  if(s.match("pull ?*")) {
    auto p = s.trimLeft("pull ", 1L).qsplit(",").strip();
    for(auto& t : p) {
      if(t == "origin") {
        origin = pushStack.takeRight().natural();
        seek(origin);
      } else if(t == "base") {
        base = pushStack.takeRight().integer();
      } else if(t == "pc") {
        base = pushStack.takeRight().integer();
        origin = pushStack.takeRight().natural();
        seek(origin);
      } else {
        error("unrecognized pull variable: ", t);
      }
    }
    return true;
  }

  //insert [name, ] filename [, offset] [, length]
  if(s.match("insert ?*")) {
    auto p = s.trimLeft("insert ", 1L).qsplit(",").strip();
    string name;
    if(!p(0).match("\"*\"")) name = p.take(0);
    if(!p(0).match("\"*\"")) error("missing filename");
    string filename = {filepath(), p.take(0).trim("\"", "\"", 1L)};
    file fp;
    if(!fp.open(filename, file::mode::read)) error("file not found: ", filename);
    uint offset = p.size() ? evaluate(p.take(0)) : 0;
    if(offset > fp.size()) offset = fp.size();
    uint length = p.size() ? evaluate(p.take(0)) : 0;
    if(length == 0) length = fp.size() - offset;
    if(name) {
      setConstant({name}, pc());
      setConstant({name, ".size"}, length);
    }
    fp.seek(offset);
    while(!fp.end() && length--) write(fp.read());
    return true;
  }

  //fill length [, with]
  if(s.match("fill ?*")) {
    auto p = s.trimLeft("fill ", 1L).qsplit(",").strip();
    uint length = evaluate(p(0));
    uint byte = evaluate(p(1, "0"));
    while(length--) write(byte);
    return true;
  }

  //map 'char' [, value] [, length]
  if(s.match("map ?*")) {
    auto p = s.trimLeft("map ", 1L).qsplit(",").strip();
    uint8_t index = evaluate(p(0));
    int64_t value = evaluate(p(1, "0"));
    int64_t length = evaluate(p(2, "1"));
    for(int n : range(length)) {
      stringTable[index + n] = value + n;
    }
    return true;
  }

  //d[bwldq] ("string"|variable) [, ...]
  uint dataLength = 0;
  if(s.beginsWith("db ")) dataLength = 1;
  if(s.beginsWith("dw ")) dataLength = 2;
  if(s.beginsWith("dl ")) dataLength = 3;
  if(s.beginsWith("dd ")) dataLength = 4;
  if(s.beginsWith("dq ")) dataLength = 8;
  if(dataLength) {
    s = slice(s, 3);  //remove prefix
    auto p = s.qsplit(",").strip();
    for(auto& t : p) {
      if(t.match("\"*\"")) {
        t = text(t);
        for(auto& b : t) write(stringTable[b], dataLength);
      } else {
        write(evaluate(t), dataLength);
      }
    }
    return true;
  }

  //print ("string"|variable) [, ...]
  if(s.match("print ?*")) {
    s.trimLeft("print ", 1L).strip();
    if(writePhase()) {
      auto p = s.qsplit(",").strip();
      for(auto& t : p) {
        if(t.match("\"*\"")) {
          print(stderr, text(t));
        } else {
          print(stderr, evaluate(t));
        }
      }
    }
    return true;
  }

  //notice "string"
  if(s.match("notice \"*\"")) {
    if(writePhase()) {
      s.trimLeft("notice ", 1L).strip();
      notice(text(s));
    }
    return true;
  }

  //warning "string"
  if(s.match("warning \"*\"")) {
    if(writePhase()) {
      s.trimLeft("warning ", 1L).strip();
      warning(text(s));
    }
    return true;
  }

  //error "string"
  if(s.match("error \"*\"")) {
    if(writePhase()) {
      s.trimLeft("error ", 1L).strip();
      error(text(s));
    }
    return true;
  }

  return false;
}
