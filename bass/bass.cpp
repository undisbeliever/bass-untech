//bass
//license: GPLv3
//author: byuu
//project started: 2013-09-27

#include "bass.hpp"
#include "core/core.cpp"
#include "arch/table/table.cpp"

#include <nall/main.hpp>
auto nall::main(string_vector args) -> void {
  if(args.size() == 1) {
    print(stderr, "bass v14.05\n");
    print(stderr, "usage: bass [options] [-o target] source [source ...]\n");
    print(stderr, "\n");
    print(stderr, "options:\n");
    print(stderr, "  -d name[=value]  create define with optional value\n");
    print(stderr, "  -c name[=value]  create constant with optional value\n");
    print(stderr, "  -strict          upgrade warnings to errors\n");
    print(stderr, "  -create          overwrite target file if it already exists\n");
    print(stderr, "  -benchmark       benchmark performance\n");
    exit(EXIT_FAILURE);
  }

  string targetFilename;
  string_vector defines;
  string_vector constants;
  bool strict = false;
  bool create = false;
  bool benchmark = false;
  string_vector sourceFilenames;

  for(uint n = 1; n < args.size();) {
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

    if(s == "-create") {
      create = true;
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
  BassTable bass;
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
