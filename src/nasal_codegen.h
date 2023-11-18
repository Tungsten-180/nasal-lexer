#pragma once

#include "nasal_err.h"
#include "nasal_opcode.h"
#include "nasal_ast.h"
#include "ast_visitor.h"
#include "symbol_finder.h"
#include "nasal_parse.h"
#include "nasal_import.h"

#include "nasal_builtin.h"
#include "coroutine.h"
#include "bits_lib.h"
#include "math_lib.h"
#include "fg_props.h"
#include "io_lib.h"
#include "dylib_lib.h"
#include "unix_lib.h"

#include <iomanip>
#include <list>
#include <stack>
#include <unordered_map>
#include <unordered_set>

#ifdef _MSC_VER
#pragma warning (disable:4244)
#pragma warning (disable:4267)
#endif

namespace nasal {

class codegen {
private:
    error err;

    // repl output flag, will generate op_repl to output stack top value if true
    bool need_repl_output;

    // file mapper for file -> index
    std::unordered_map<std::string, usize> file_map;
    void init_file_map(const std::vector<std::string>&);

    // used for generate pop in return expression
    std::vector<u32> in_foreach_loop_level;

    // constant numbers and strings
    std::unordered_map<f64, u32> const_number_map;
    std::unordered_map<std::string, u32> const_string_map;
    std::vector<f64> const_number_table;
    std::vector<std::string> const_string_table;

    // native functions
    std::vector<nasal_builtin_table> native_function;
    std::unordered_map<std::string, usize> native_function_mapper;
    void load_native_function_table(nasal_builtin_table*);
    void init_native_function();

    // generated opcodes
    std::vector<opcode> code;

    // used to store jmp operands index, to fill the jump address back
    std::list<std::vector<i32>> continue_ptr;
    std::list<std::vector<i32>> break_ptr;

    // symbol table
    // global : max STACK_DEPTH-1 values
    std::unordered_map<std::string, i32> global;
    std::unordered_map<std::string, std::unordered_set<std::string>> experimental_namespace;

    // local  : max 32768 upvalues 65536 values
    // but in fact local scope also has less than STACK_DEPTH value
    std::list<std::unordered_map<std::string, i32>> local;

    void check_id_exist(identifier*);
    
    void die(const std::string& info, const span& loc) {
        err.err("code", loc, info);
    }

    void regist_num(const f64);
    void regist_str(const std::string&);
    void find_symbol(code_block*);
    void add_symbol(const std::string&);
    i32 local_symbol_find(const std::string&);
    i32 global_symbol_find(const std::string&);
    i32 upvalue_symbol_find(const std::string&);

    void emit(u8, u32, const span&);

    void num_gen(number_literal*);
    void str_gen(string_literal*);
    void bool_gen(bool_literal*);
    void vec_gen(vector_expr*);
    void hash_gen(hash_expr*);
    void func_gen(function*);
    void call_gen(call_expr*);
    void call_id(identifier*);
    void call_hash_gen(call_hash*);
    void call_vector_gen(call_vector*);
    void call_func_gen(call_function*);
    void mcall(expr*);
    void mcall_id(identifier*);
    void mcall_vec(call_vector*);
    void mcall_hash(call_hash*);
    void multi_def(definition_expr*);
    void single_def(definition_expr*);
    void def_gen(definition_expr*);
    void assignment_expression(assignment_expr*);
    void gen_assignment_equal_statement(assignment_expr*);
    void replace_left_assignment_with_load(const span&);
    void assignment_statement(assignment_expr*);
    void multi_assign_gen(multi_assign*);
    void cond_gen(condition_expr*);
    void loop_gen(expr*);
    void load_continue_break(i32, i32);
    void while_gen(while_expr*);
    void for_gen(for_expr*);
    void forei_gen(forei_expr*);
    void statement_generation(expr*);
    void or_gen(binary_operator*);
    void and_gen(binary_operator*);
    void unary_gen(unary_operator*);
    void binary_gen(binary_operator*);
    void trino_gen(ternary_operator*);
    void calc_gen(expr*);
    void repl_mode_info_output_gen(expr*);
    void block_gen(code_block*);
    void ret_gen(return_expr*);

public:
    const auto& strs() const {return const_string_table;}
    const auto& nums() const {return const_number_table;}
    const auto& natives() const {return native_function;}
    const auto& codes() const {return code;}
    const auto& globals() const {return global;}
    const auto& get_experimental_namespace() const {
        return experimental_namespace;
    }

public:
    codegen() = default;
    const error& compile(parse&, linker&, bool);
    void print(std::ostream&);
    void symbol_dump(std::ostream&) const;
};

}
