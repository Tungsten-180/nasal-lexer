#pragma once

#include "nasal.h"
#include "nasal_err.h"

#include <unordered_map>
#include <vector>

namespace nasal {

enum class expr_type : u32 {
  ast_null = 0,     // null node
  ast_block,        // code block
  ast_nil,          // nil keyword
  ast_num,          // number, basic value type
  ast_str,          // string, basic value type
  ast_id,           // identifier
  ast_bool,         // bools
  ast_func,         // func keyword
  ast_hash,         // hash, basic value type
  ast_vec,          // vector, basic value type
  ast_pair,         // pair of key and value in hashmap
  ast_call,         // mark a sub-tree of calling an identifier
  ast_callh,        // id.name
  ast_callv,        // id[index]
  ast_callf,        // id()
  ast_subvec,       // id[index:index]
  ast_param,        // function parameter
  ast_ternary,      // ternary operator
  ast_binary,       // binary operator
  ast_unary,        // unary operator
  ast_for,          // for keyword
  ast_forei,        // foreach or forindex loop
  ast_while,        // while
  ast_iter,         // iterator, used in forindex/foreach
  ast_cond,         // mark a sub-tree of conditional expression
  ast_if,           // if keyword
  ast_multi_id,     // multi identifiers sub-tree
  ast_tuple,        // tuple, stores multiple scalars
  ast_def,          // definition
  ast_assign,       // assignment
  ast_multi_assign, // multiple assignment
  ast_continue,     // continue keyword, only used in loop
  ast_break,        // break keyword, only used in loop
  ast_ret           // return keyword, only used in function block
};

class ast_visitor;
class hash_pair;
class parameter;
class slice_vector;
class multi_identifier;
class code_block;
class if_expr;
class tuple_expr;

class expr {
protected:
  span nd_loc;
  expr_type nd_type;

public:
  expr(const span &location, expr_type node_type)
      : nd_loc(location), nd_type(node_type) {}
  virtual ~expr() = default;
  void set_begin(u32 line, u32 column) {
    nd_loc.begin_line = line;
    nd_loc.begin_column = column;
  }
  const span &get_location() const { return nd_loc; }
  const u32 get_line() const { return nd_loc.begin_line; }
  expr_type get_type() const { return nd_type; }
  void update_location(const span &location) {
    nd_loc.end_line = location.end_line;
    nd_loc.end_column = location.end_column;
  }
  virtual void accept(ast_visitor *);
};

class call : public expr {
public:
  call(const span &location, expr_type node_type) : expr(location, node_type) {}
  virtual ~call() = default;
  virtual void accept(ast_visitor *);
};

class null_expr : public expr {
public:
  null_expr(const span &location) : expr(location, expr_type::ast_null) {}
  ~null_expr() override = default;
  void accept(ast_visitor *) override;
};

class nil_expr : public expr {
public:
  nil_expr(const span &location) : expr(location, expr_type::ast_nil) {}
  ~nil_expr() override = default;
  void accept(ast_visitor *) override;
};

class number_literal : public expr {
private:
  f64 number;

public:
  number_literal(const span &location, const f64 num)
      : expr(location, expr_type::ast_num), number(num) {}
  ~number_literal() override = default;
  f64 get_number() const { return number; }
  void accept(ast_visitor *) override;
};

class string_literal : public expr {
private:
  std::string content;

public:
  string_literal(const span &location, const std::string &str)
      : expr(location, expr_type::ast_str), content(str) {}
  ~string_literal() override = default;
  const std::string get_content() const { return content; }
  void accept(ast_visitor *) override;
};

class identifier : public expr {
private:
  std::string name;

public:
  identifier(const span &location, const std::string &str)
      : expr(location, expr_type::ast_id), name(str) {}
  ~identifier() override = default;
  const std::string &get_name() const { return name; }
  void accept(ast_visitor *) override;
};

class bool_literal : public expr {
private:
  bool flag;

public:
  bool_literal(const span &location, const bool bool_flag)
      : expr(location, expr_type::ast_bool), flag(bool_flag) {}
  ~bool_literal() override = default;
  bool get_flag() const { return flag; }
  void accept(ast_visitor *) override;
};

class vector_expr : public expr {
private:
  std::vector<expr *> elements;

public:
  vector_expr(const span &location) : expr(location, expr_type::ast_vec) {}
  ~vector_expr() override;
  void add_element(expr *node) { elements.push_back(node); }
  std::vector<expr *> &get_elements() { return elements; }
  void accept(ast_visitor *) override;
};

class hash_expr : public expr {
private:
  std::vector<hash_pair *> members;

public:
  hash_expr(const span &location) : expr(location, expr_type::ast_hash) {}
  ~hash_expr() override;
  void add_member(hash_pair *node) { members.push_back(node); }
  std::vector<hash_pair *> &get_members() { return members; }
  void accept(ast_visitor *) override;
};

class hash_pair : public expr {
private:
  std::string name;
  expr *value;

public:
  hash_pair(const span &location)
      : expr(location, expr_type::ast_pair), value(nullptr) {}
  ~hash_pair() override;
  void set_name(const std::string &field_name) { name = field_name; }
  void set_value(expr *node) { value = node; }
  const std::string &get_name() const { return name; }
  expr *get_value() { return value; }
  void accept(ast_visitor *) override;
};

class function : public expr {
private:
  std::vector<parameter *> parameter_list;
  code_block *block;

public:
  function(const span &location)
      : expr(location, expr_type::ast_func), block(nullptr) {}
  ~function() override;
  void add_parameter(parameter *node) { parameter_list.push_back(node); }
  void set_code_block(code_block *node) { block = node; }
  std::vector<parameter *> &get_parameter_list() { return parameter_list; }
  code_block *get_code_block() { return block; }
  void accept(ast_visitor *) override;
};

class code_block : public expr {
private:
  std::vector<expr *> expressions;

public:
  code_block(const span &location) : expr(location, expr_type::ast_block) {}
  ~code_block() override;
  void add_expression(expr *node) { expressions.push_back(node); }
  std::vector<expr *> &get_expressions() { return expressions; }
  void accept(ast_visitor *) override;
};

class parameter : public expr {
public:
  enum class param_type {
    normal_parameter,
    default_parameter,
    dynamic_parameter
  };

private:
  param_type type;
  std::string name;
  expr *default_value;

public:
  parameter(const span &location)
      : expr(location, expr_type::ast_param), name(""), default_value(nullptr) {
  }
  ~parameter() override;
  void set_parameter_type(param_type pt) { type = pt; }
  void set_parameter_name(const std::string &pname) { name = pname; }
  void set_default_value(expr *node) { default_value = node; }
  param_type get_parameter_type() { return type; }
  const std::string &get_parameter_name() const { return name; }
  expr *get_default_value() { return default_value; }
  void accept(ast_visitor *) override;
};

class ternary_operator : public expr {
private:
  expr *condition;
  expr *left;
  expr *right;

public:
  ternary_operator(const span &location)
      : expr(location, expr_type::ast_ternary), condition(nullptr),
        left(nullptr), right(nullptr) {}
  ~ternary_operator() override;
  void set_condition(expr *node) { condition = node; }
  void set_left(expr *node) { left = node; }
  void set_right(expr *node) { right = node; }
  expr *get_condition() { return condition; }
  expr *get_left() { return left; }
  expr *get_right() { return right; }
  void accept(ast_visitor *) override;
};

class binary_operator : public expr {
public:
  enum class binary_type {
    add,
    sub,
    mult,
    div,
    concat,
    cmpeq,
    cmpneq,
    less,
    leq,
    grt,
    geq,
    bitwise_or,
    bitwise_xor,
    bitwise_and,
    condition_and,
    condition_or
  };

private:
  binary_type type;
  expr *left;
  expr *right;
  number_literal *optimized_const_number;
  string_literal *optimized_const_string;

public:
  binary_operator(const span &location)
      : expr(location, expr_type::ast_binary), left(nullptr), right(nullptr),
        optimized_const_number(nullptr), optimized_const_string(nullptr) {}
  ~binary_operator() override;
  void set_operator_type(binary_type operator_type) { type = operator_type; }
  void set_left(expr *node) { left = node; }
  void set_right(expr *node) { right = node; }
  void set_optimized_number(number_literal *node) {
    optimized_const_number = node;
  }
  void set_optimized_string(string_literal *node) {
    optimized_const_string = node;
  }
  binary_type get_operator_type() const { return type; }
  expr *get_left() { return left; }
  expr *get_right() { return right; }
  number_literal *get_optimized_number() { return optimized_const_number; }
  string_literal *get_optimized_string() { return optimized_const_string; }
  void accept(ast_visitor *) override;
};

class unary_operator : public expr {
public:
  enum class unary_type { negative, logical_not, bitwise_not };

private:
  unary_type type;
  expr *value;
  number_literal *optimized_number;

public:
  unary_operator(const span &location)
      : expr(location, expr_type::ast_unary), value(nullptr),
        optimized_number(nullptr) {}
  ~unary_operator() override;
  void set_operator_type(unary_type operator_type) { type = operator_type; }
  void set_value(expr *node) { value = node; }
  void set_optimized_number(number_literal *node) { optimized_number = node; }
  unary_type get_operator_type() const { return type; }
  expr *get_value() { return value; }
  number_literal *get_optimized_number() { return optimized_number; }
  void accept(ast_visitor *) override;
};

class call_expr : public expr {
private:
  expr *first;
  std::vector<call *> calls;

public:
  call_expr(const span &location)
      : expr(location, expr_type::ast_call), first(nullptr) {}
  ~call_expr() override;
  void set_first(expr *node) { first = node; }
  void add_call(call *node) { calls.push_back(node); }
  expr *get_first() { return first; }
  auto &get_calls() { return calls; }
  void accept(ast_visitor *) override;
};

class call_hash : public call {
private:
  std::string field;

public:
  call_hash(const span &location, const std::string &name)
      : call(location, expr_type::ast_callh), field(name) {}
  ~call_hash() override = default;
  const std::string &get_field() const { return field; }
  void accept(ast_visitor *) override;
};

class call_vector : public call {
private:
  std::vector<slice_vector *> calls;

public:
  call_vector(const span &location) : call(location, expr_type::ast_callv) {}
  ~call_vector() override;
  void add_slice(slice_vector *node) { calls.push_back(node); }
  std::vector<slice_vector *> &get_slices() { return calls; }
  void accept(ast_visitor *) override;
};

class call_function : public call {
private:
  std::vector<expr *> args;

public:
  call_function(const span &location) : call(location, expr_type::ast_callf) {}
  ~call_function() override;
  void add_argument(expr *node) { args.push_back(node); }
  std::vector<expr *> &get_argument() { return args; }
  void accept(ast_visitor *) override;
};

class slice_vector : public expr {
private:
  expr *begin;
  expr *end;

public:
  slice_vector(const span &location)
      : expr(location, expr_type::ast_subvec), begin(nullptr), end(nullptr) {}
  ~slice_vector() override;
  void set_begin(expr *node) { begin = node; }
  void set_end(expr *node) { end = node; }
  expr *get_begin() { return begin; }
  expr *get_end() { return end; }
  void accept(ast_visitor *) override;
};

class definition_expr : public expr {
private:
  identifier *variable_name;
  multi_identifier *variables;
  tuple_expr *tuple;
  expr *value;

public:
  definition_expr(const span &location)
      : expr(location, expr_type::ast_def), variable_name(nullptr),
        variables(nullptr), tuple(nullptr), value(nullptr) {}
  ~definition_expr() override;
  void set_identifier(identifier *node) { variable_name = node; }
  void set_multi_define(multi_identifier *node) { variables = node; }
  void set_tuple(tuple_expr *node) { tuple = node; }
  void set_value(expr *node) { value = node; }
  identifier *get_variable_name() { return variable_name; }
  multi_identifier *get_variables() { return variables; }
  tuple_expr *get_tuple() { return tuple; }
  expr *get_value() { return value; }
  void accept(ast_visitor *) override;
};

class assignment_expr : public expr {
public:
  enum class assign_type {
    equal,
    add_equal,
    sub_equal,
    mult_equal,
    div_equal,
    concat_equal,
    bitwise_and_equal,
    bitwise_or_equal,
    bitwise_xor_equal
  };

private:
  assign_type type;
  expr *left;
  expr *right;

public:
  assignment_expr(const span &location)
      : expr(location, expr_type::ast_assign), left(nullptr), right(nullptr) {}
  ~assignment_expr() override;
  void set_assignment_type(assign_type operator_type) { type = operator_type; }
  void set_left(expr *node) { left = node; }
  void set_right(expr *node) { right = node; }
  assign_type get_assignment_type() const { return type; }
  expr *get_left() { return left; }
  expr *get_right() { return right; }
  void accept(ast_visitor *) override;
};

class multi_identifier : public expr {
private:
  std::vector<identifier *> variables;

public:
  multi_identifier(const span &location)
      : expr(location, expr_type::ast_multi_id) {}
  ~multi_identifier() override;
  void add_var(identifier *node) { variables.push_back(node); }
  std::vector<identifier *> &get_variables() { return variables; }
  void accept(ast_visitor *) override;
};

class tuple_expr : public expr {
private:
  std::vector<expr *> elements;

public:
  tuple_expr(const span &location) : expr(location, expr_type::ast_tuple) {}
  ~tuple_expr() override;
  void add_element(expr *node) { elements.push_back(node); }
  std::vector<expr *> &get_elements() { return elements; }
  void accept(ast_visitor *) override;
};

class multi_assign : public expr {
private:
  tuple_expr *tuple;
  expr *value;

public:
  multi_assign(const span &location)
      : expr(location, expr_type::ast_multi_assign), tuple(nullptr),
        value(nullptr) {}
  ~multi_assign() override;
  void set_tuple(tuple_expr *node) { tuple = node; }
  void set_value(expr *node) { value = node; }
  tuple_expr *get_tuple() { return tuple; }
  expr *get_value() { return value; }
  void accept(ast_visitor *) override;
};

class while_expr : public expr {
private:
  expr *condition;
  code_block *block;

public:
  while_expr(const span &location)
      : expr(location, expr_type::ast_while), condition(nullptr),
        block(nullptr) {}
  ~while_expr() override;
  void set_condition(expr *node) { condition = node; }
  void set_code_block(code_block *node) { block = node; }
  expr *get_condition() { return condition; }
  code_block *get_code_block() { return block; }
  void accept(ast_visitor *) override;
};

class for_expr : public expr {
private:
  expr *initializing;
  expr *condition;
  expr *step;
  code_block *block;

public:
  for_expr(const span &location)
      : expr(location, expr_type::ast_for), initializing(nullptr),
        condition(nullptr), step(nullptr), block(nullptr) {}
  ~for_expr() override;
  void set_initial(expr *node) { initializing = node; }
  void set_condition(expr *node) { condition = node; }
  void set_step(expr *node) { step = node; }
  void set_code_block(code_block *node) { block = node; }
  expr *get_initial() { return initializing; }
  expr *get_condition() { return condition; }
  expr *get_step() { return step; }
  code_block *get_code_block() { return block; }
  void accept(ast_visitor *) override;
};

class iter_expr : public expr {
private:
  bool is_iterator_definition;
  identifier *name;
  call_expr *call;

public:
  iter_expr(const span &location)
      : expr(location, expr_type::ast_iter), is_iterator_definition(false),
        name(nullptr), call(nullptr) {}
  ~iter_expr() override;
  void set_name(identifier *node) { name = node; }
  void set_call(call_expr *node) { call = node; }
  void set_is_definition(bool flag) { is_iterator_definition = flag; }
  identifier *get_name() { return name; }
  call_expr *get_call() { return call; }
  bool is_definition() const { return is_iterator_definition; }
  void accept(ast_visitor *) override;
};

class forei_expr : public expr {
public:
  enum class forei_loop_type { foreach, forindex };

private:
  forei_loop_type type;
  iter_expr *iterator;
  expr *vector_node;
  code_block *block;

public:
  forei_expr(const span &location)
      : expr(location, expr_type::ast_forei), type(forei_loop_type::foreach),
        iterator(nullptr), vector_node(nullptr), block(nullptr) {}
  ~forei_expr() override;
  void set_loop_type(forei_loop_type ft) { type = ft; }
  void set_iterator(iter_expr *node) { iterator = node; }
  void set_value(expr *node) { vector_node = node; }
  void set_code_block(code_block *node) { block = node; }
  forei_loop_type get_loop_type() const { return type; }
  iter_expr *get_iterator() { return iterator; }
  expr *get_value() { return vector_node; }
  code_block *get_code_block() { return block; }
  void accept(ast_visitor *) override;
};

class condition_expr : public expr {
private:
  if_expr *if_stmt;
  std::vector<if_expr *> elsif_stmt;
  if_expr *else_stmt;

public:
  condition_expr(const span &location)
      : expr(location, expr_type::ast_cond), if_stmt(nullptr),
        else_stmt(nullptr) {}
  ~condition_expr() override;
  void set_if_statement(if_expr *node) { if_stmt = node; }
  void add_elsif_statement(if_expr *node) { elsif_stmt.push_back(node); }
  void set_else_statement(if_expr *node) { else_stmt = node; }
  if_expr *get_if_statement() { return if_stmt; }
  std::vector<if_expr *> &get_elsif_stataments() { return elsif_stmt; }
  if_expr *get_else_statement() { return else_stmt; }
  void accept(ast_visitor *) override;
};

class if_expr : public expr {
private:
  expr *condition;
  code_block *block;

public:
  if_expr(const span &location)
      : expr(location, expr_type::ast_if), condition(nullptr), block(nullptr) {}
  ~if_expr() override;
  void set_condition(expr *node) { condition = node; }
  void set_code_block(code_block *node) { block = node; }
  expr *get_condition() { return condition; }
  code_block *get_code_block() { return block; }
  void accept(ast_visitor *) override;
};

class continue_expr : public expr {
public:
  continue_expr(const span &location)
      : expr(location, expr_type::ast_continue) {}
  ~continue_expr() override = default;
  void accept(ast_visitor *) override;
};

class break_expr : public expr {
public:
  break_expr(const span &location) : expr(location, expr_type::ast_break) {}
  ~break_expr() = default;
  void accept(ast_visitor *) override;
};

class return_expr : public expr {
private:
  expr *value;

public:
  return_expr(const span &location)
      : expr(location, expr_type::ast_ret), value(nullptr) {}
  ~return_expr() override;
  void set_value(expr *node) { value = node; }
  expr *get_value() { return value; }
  void accept(ast_visitor *) override;
};

} // namespace nasal
