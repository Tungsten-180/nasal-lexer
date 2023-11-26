#include "ast_dumper.h"
#include "ast_visitor.h"
#include "nasal.h"
#include "nasal_ast.h"
#include "nasal_codegen.h"
#include "nasal_dbg.h"
#include "nasal_err.h"
#include "nasal_gc.h"
#include "nasal_import.h"
#include "nasal_lexer.h"
#include "nasal_parse.h"
#include "nasal_type.h"
#include "nasal_vm.h"
#include "optimizer.h"
#include "repl.h"
#include "symbol_finder.h"

#include <cstdlib>
#include <iostream>
#include <thread>
#include <unordered_map>

void parse(const std::string &file, const std::string &filesname) {

  using clk = std::chrono::high_resolution_clock;
  const auto den = clk::duration::period::den;

  nasal::lexer lex;
  nasal::parse parse;

  // lexer scans file from stdin to get tokens
  lex.sscan(file, filesname).chkerr();

  // parser gets lexer's token list to compile
  // will send info to stdout
  parse.compile(lex).chkerr();
}

i32 main(i32 argc, const char *argv[]) {
    std::string line = "";
    std::string filesname = "";
    if (argc >= 2) {
        if (*argv[1] == 'n'){
            std::getline(std::cin, filesname);
        }
    }
    std::string *file = new std::string("");
    while (std::getline(std::cin, line)) {
      *file += line;
      *file += "\n";
    }
    parse(*file, filesname);
    return 0;
}
