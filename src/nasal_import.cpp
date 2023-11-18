#include "nasal_import.h"
#include "symbol_finder.h"

#include <memory>

namespace nasal {

linker::linker():
    show_path(false), lib_loaded(false),
    this_file(""), lib_path("") {
    char sep = is_windows()? ';':':';
    std::string PATH = getenv("PATH");
    usize last = 0, pos = PATH.find(sep, 0);
    while(pos!=std::string::npos) {
        std::string dirpath = PATH.substr(last, pos-last);
        if (dirpath.length()) {
            envpath.push_back(dirpath);
        }
        last = pos+1;
        pos = PATH.find(sep, last);
    }
    if (last!=PATH.length()) {
        envpath.push_back(PATH.substr(last));
    }
}

std::string linker::get_path(call_expr* node) {
    if (node->get_calls()[0]->get_type()==expr_type::ast_callf) {
        auto tmp = (call_function*)node->get_calls()[0];
        return ((string_literal*)tmp->get_argument()[0])->get_content();
    }
    auto fpath = std::string(".");
    for(auto i : node->get_calls()) {
        fpath += (is_windows()? "\\":"/") + ((call_hash*)i)->get_field();
    }
    return fpath + ".nas";
}

std::string linker::find_file(
    const std::string& filename, const span& location) {
    // first add file name itself into the file path
    std::vector<std::string> fpath = {filename};

    // generate search path from environ path
    for(const auto& p : envpath) {
        fpath.push_back(p + (is_windows()? "\\":"/") + filename);
    }

    // search file
    for(const auto& i : fpath) {
        if (access(i.c_str(), F_OK)!=-1) {
            return i;
        }
    }

    // we will find lib.nas in nasal std directory
    if (filename=="lib.nas") {
        return is_windows()?
            find_file("std\\lib.nas", location):
            find_file("std/lib.nas", location);
    }
    if (!show_path) {
        err.err("link",
            "in <" + location.file + ">: " +
            "cannot find file <" + filename + ">, " +
            "use <-d> to get detail search path");
        return "";
    }
    std::string paths = "";
    for(const auto& i : fpath) {
        paths += "  -> " + i + "\n";
    }
    err.err("link",
        "in <" + location.file + ">: " +
        "cannot find file <" + filename + "> in these paths:\n" + paths);
    return "";
}

bool linker::import_check(expr* node) {
/*
    call
    |_id:import
    |_callh:std
    |_callh:file
*/
    if (node->get_type()!=expr_type::ast_call) {
        return false;
    }
    auto tmp = (call_expr*)node;
    if (tmp->get_first()->get_type()!=expr_type::ast_id) {
        return false;
    }
    if (((identifier*)tmp->get_first())->get_name()!="import") {
        return false;
    }
    if (!tmp->get_calls().size()) {
        return false;
    }
    // import.xxx.xxx;
    if (tmp->get_calls()[0]->get_type()==expr_type::ast_callh) {
        for(auto i : tmp->get_calls()) {
            if (i->get_type()!=expr_type::ast_callh) {
                return false;
            }
        }
        return true;
    }
    // import("xxx");
    if (tmp->get_calls().size()!=1) {
        return false;
    }
/*
    call
    |_id:import
    |_call_func
      |_string:'filename'
*/
    if (tmp->get_calls()[0]->get_type()!=expr_type::ast_callf) {
        return false;
    }
    auto func_call = (call_function*)tmp->get_calls()[0];
    if (func_call->get_argument().size()!=1) {
        return false;
    }
    if (func_call->get_argument()[0]->get_type()!=expr_type::ast_str) {
        return false;
    }
    return true;
}

bool linker::exist(const std::string& file) {
    // avoid importing the same file
    for(const auto& fname : files) {
        if (file==fname) {
            return true;
        }
    }
    files.push_back(file);
    return false;
}

u16 linker::find(const std::string& file) {
    for(usize i = 0; i<files.size(); ++i) {
        if (files[i]==file) {
            return static_cast<u16>(i);
        }
    }
    std::cerr << "unreachable: using this method incorrectly\n";
    std::exit(-1);
    return UINT16_MAX;
}

bool linker::check_self_import(const std::string& file) {
    for(const auto& i : module_load_stack) {
        if (file==i) {
            return true;
        }
    }
    return false;
}

std::string linker::generate_self_import_path(const std::string& filename) {
    std::string res = "";
    for(const auto& i : module_load_stack) {
        res += "[" + i + "] -> ";
    }
    return res + "[" + filename + "]";
}

void linker::link(code_block* new_tree_root, code_block* old_tree_root) {
    // add children of add_root to the back of root
    for(auto& i : old_tree_root->get_expressions()) {
        new_tree_root->add_expression(i);
    }
    // clean old root
    old_tree_root->get_expressions().clear();
}

code_block* linker::import_regular_file(call_expr* node) {
    lexer lex;
    parse par;
    // get filename
    auto filename = get_path(node);
    // clear this node
    for(auto i : node->get_calls()) {
        delete i;
    }
    node->get_calls().clear();
    auto location = node->get_first()->get_location();
    delete node->get_first();
    node->set_first(new nil_expr(location));
    // this will make node to call_expr(nil),
    // will not be optimized when generating bytecodes

    // avoid infinite loading loop
    filename = find_file(filename, node->get_location());
    if (!filename.length()) {
        return new code_block({0, 0, 0, 0, filename});
    }
    if (check_self_import(filename)) {
        err.err("link",
            "self-referenced module <" + filename + ">:\n" +
            "    reference path: " + generate_self_import_path(filename)
        );
        return new code_block({0, 0, 0, 0, filename});
    }
    exist(filename);
    
    module_load_stack.push_back(filename);
    // start importing...
    if (lex.scan(filename).geterr()) {
        err.err("link", "error occurred when analysing <" + filename + ">");
        return new code_block({0, 0, 0, 0, filename});
    }
    if (par.compile(lex).geterr())  {
        err.err("link", "error occurred when analysing <" + filename + ">");
        return new code_block({0, 0, 0, 0, filename});
    }

    auto parse_result = par.swap(nullptr);

    // check if parse result has 'import'
    auto result = load(parse_result, find(filename));
    module_load_stack.pop_back();
    return result;
}

code_block* linker::import_nasal_lib() {
    lexer lex;
    parse par;
    auto filename = find_file("lib.nas", {0, 0, 0, 0, files[0]});
    if (!filename.length()) {
        return new code_block({0, 0, 0, 0, filename});
    }
    lib_path = filename;

    // avoid infinite loading library
    if (exist(filename)) {
        return new code_block({0, 0, 0, 0, filename});
    }
    
    // start importing...
    if (lex.scan(filename).geterr()) {
        err.err("link",
            "error occurred when analysing library <" + filename + ">"
        );
        return new code_block({0, 0, 0, 0, filename});
    }
    if (par.compile(lex).geterr())  {
        err.err("link",
            "error occurred when analysing library <" + filename + ">"
        );
        return new code_block({0, 0, 0, 0, filename});
    }

    auto parse_result = par.swap(nullptr);
    // check if library has 'import' (in fact it should not)
    return load(parse_result, find(filename));
}

std::string linker::generate_module_name(const std::string& file_path) {
    auto error_name = "module@[" + file_path + "]";
    if (!file_path.length()) {
        return error_name;
    }

    // check file suffix and get file suffix position
    auto suffix_position = file_path.find(".nas");
    if (suffix_position==std::string::npos) {
        err.warn("link",
            "get invalid module name from <" + file_path + ">, " +
            "will not be easily accessed. " +
            "\".nas\" suffix is required."
        );
        return error_name;
    }
    if (suffix_position+4!=file_path.length()) {
        err.warn("link",
            "get invalid module name from <" + file_path + ">, " +
            "will not be easily accessed. " +
            "only one \".nas\" suffix is required in the path."
        );
        return error_name;
    }

    // only get the file name as module name, directory path is not included
    auto split_position = file_path.find_last_of("/");
    // find "\\" in windows platform
    if (split_position==std::string::npos) {
        split_position = file_path.find_last_of("\\");
    }

    // split file path to get module name
    auto module_name = split_position==std::string::npos?
        file_path.substr(0, suffix_position):
        file_path.substr(split_position+1, suffix_position-split_position-1);
    
    // check validation of module name
    if (!module_name.length()) {
        err.warn("link",
            "get empty module name from <" + file_path + ">, " +
            "will not be easily accessed."
        );
    }
    if (module_name.length() && '0' <= module_name[0] && module_name[0] <= '9') {
        err.warn("link",
            "get module <" + module_name + "> from <" + file_path + ">, " +
            "will not be easily accessed."
        );
    }
    if (module_name.length() && module_name.find(".")!=std::string::npos) {
        err.warn("link",
            "get module <" + module_name + "> from <" + file_path + ">, " +
            "will not be easily accessed."
        );
    }
    return module_name;
}

return_expr* linker::generate_module_return(code_block* block) {
    auto finder = std::unique_ptr<symbol_finder>(new symbol_finder);
    auto result = new return_expr(block->get_location());
    auto value = new hash_expr(block->get_location());
    result->set_value(value);
    for(const auto& i : finder->do_find(block)) {
        auto pair = new hash_pair(block->get_location());
        // do not export symbol begins with '_'
        if (i.name.length() && i.name[0]=='_') {
            continue;
        }
        pair->set_name(i.name);
        pair->set_value(new identifier(block->get_location(), i.name));
        value->add_member(pair);
    }
    return result;
}

definition_expr* linker::generate_module_definition(code_block* block) {
    auto def = new definition_expr(block->get_location());
    def->set_identifier(new identifier(
        block->get_location(),
        generate_module_name(block->get_location().file)
    ));
    
    auto call = new call_expr(block->get_location());
    auto func = new function(block->get_location());
    func->set_code_block(block);
    func->get_code_block()->add_expression(generate_module_return(block));
    call->set_first(func);
    call->add_call(new call_function(block->get_location()));

    def->set_value(call);
    return def;
}

code_block* linker::load(code_block* program_root, u16 fileindex) {
    auto tree = new code_block({0, 0, 0, 0, files[fileindex]});
    // load library, this ast will be linked with root directly
    // so no extra namespace is generated
    if (!lib_loaded) {
        auto nasal_lib_code_block = import_nasal_lib();
        // insert nasal lib code to the back of tree
        link(tree, nasal_lib_code_block);
        delete nasal_lib_code_block;
        lib_loaded = true;
    }

    // load imported modules
    for(auto& import_ast_node : program_root->get_expressions()) {
        if (!import_check(import_ast_node)) {
            break;
        }
        auto module_code_block = import_regular_file((call_expr*)import_ast_node);
        // after importing the regular file as module, delete this node
        const auto loc = import_ast_node->get_location();
        delete import_ast_node;
        // and replace the node with null_expr node
        import_ast_node = new null_expr(loc);
        // then we generate a function warping the code block,
        // and export the necessary global symbols in this code block
        // by generate a return statement, with a hashmap return value
        tree->add_expression(generate_module_definition(module_code_block));
    }

    // insert program root to the back of tree
    link(tree, program_root);
    return tree;
}

const error& linker::link(
    parse& parse, const std::string& self, bool spath = false) {
    show_path = spath;
    // initializing file map
    this_file = self;
    files = {self};
    module_load_stack = {self};
    // scan root and import files
    // then generate a new ast and return to import_ast
    // the main file's index is 0
    auto new_tree_root = load(parse.tree(), 0);
    auto old_tree_root = parse.swap(new_tree_root);
    delete old_tree_root;
    return err;
}

}
