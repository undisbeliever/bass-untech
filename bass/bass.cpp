//bass
//license: GPLv3
//author: byuu
//project started: 2013-09-27

#include "bass.hpp"
#include "core/core.cpp"
#include "architecture/table/table.cpp"

#include <nall/main.hpp>
auto nall::main(string_vector args) -> void {
  if(args.size() < 3 || (args[1] != "create" && args[1] != "modify")) {
    print(stderr, "bass v14.10\n");
    print(stderr, "usage: bass (create|modify) [options] source [source ...]\n");
    print(stderr, "\n");
    print(stderr, "options:\n");
    print(stderr, "  -o target        specify default output filename\n");
    print(stderr, "  -d name[=value]  create define with optional value\n");
    print(stderr, "  -c name[=value]  create constant with optional value\n");
    print(stderr, "  -strict          upgrade warnings to errors\n");
    print(stderr, "  -benchmark       benchmark performance\n");
    exit(EXIT_FAILURE);
  }

  string targetFilename;
  string_vector defines;
  string_vector constants;
  bool create = args[1] == "create";
  bool strict = false;
  bool benchmark = false;
  string_vector sourceFilenames;

  for(uint n = 2; n < args.size();) {
    string s = args[n];

    if(s == "-o") {
      targetFilename = args(n + 1, "");
      n += 2;
      continue;
    }

    if(s == "-d") {
      defines.append(args(n + 1, ""));
      n += 2;
      continue;
    }

    if(s == "-c") {
      constants.append(args(n + 1, ""));
      n += 2;
      continue;
    }

    if(s == "-strict") {
      strict = true;
      n += 1;
      continue;
    }

    if(s == "-benchmark") {
      benchmark = true;
      n += 1;
      continue;
    }

    if(!s.beginsWith("-")) {
      sourceFilenames.append(s);
      n += 1;
      continue;
    }

    print(stderr, "error: unrecognized argument: ", s, "\n");
    exit(EXIT_FAILURE);
  }

  clock_t clockStart = clock();
  Bass bass;
  bass.target(targetFilename, create);
  for(auto& sourceFilename : sourceFilenames) {
    bass.source(sourceFilename);
  }
  for(auto& define : defines) {
    auto p = define.split("=", 1L);
    bass.define(p(0), p(1));
  }
  for(auto& constant : constants) {
    auto p = constant.split("=", 1L);
    bass.constant(p(0), p(1, "1"));
  }
  if(!bass.assemble(strict)) {
    print(stderr, "bass: assembly failed\n");
    exit(EXIT_FAILURE);
  }
  clock_t clockFinish = clock();
  if(benchmark) {
    print(stderr, "bass: assembled in ", (double)(clockFinish - clockStart) / CLOCKS_PER_SEC, " seconds\n");
  }
}
