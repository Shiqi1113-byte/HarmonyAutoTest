#include "ModulesManager.h"
#include <iostream>
#include <sstream>
#include <cstring>


extern "C" const TSLanguage *tree_sitter_typescript(void);

std::string ModulesManager::FileNode::getNodeText(TSNode node, const std::string& source_code) {
    if (ts_node_is_null(node)) return "";
    uint32_t start = ts_node_start_byte(node);
    uint32_t end = ts_node_end_byte(node);
    if (start >= source_code.length() || end > source_code.length() || start >= end) {
        return "";
    }
    return source_code.substr(start, end - start);
}

std::string ModulesManager::FileNode::stripQuotes(const std::string& str) {
    if (str.length() >= 2 && (str.front() == '"' || str.front() == '\'') && (str.back() == '"' || str.back() == '\'')) {
        return str.substr(1, str.length() - 2);
    }
    return str;
}

void ModulesManager::FileNode::traverseAST(TSNode node, const std::string& source_code) {
    if (ts_node_is_null(node)) return;

    const char* node_type = ts_node_type(node);
    std::string type_str(node_type);

    // 1. 处理 Imports
    if (type_str == "import_statement") {
        std::string import_source = "";
        std::vector<std::string> imported_symbols;

        uint32_t child_count = ts_node_child_count(node);
        for (uint32_t i = 0; i < child_count; i++) {
            TSNode child = ts_node_child(node, i);
            std::string child_type(ts_node_type(child));

            if (child_type == "string") {
                import_source = stripQuotes(getNodeText(child, source_code));
            } else if (child_type == "import_clause") {
                uint32_t grand_child_count = ts_node_child_count(child);
                for (uint32_t j = 0; j < grand_child_count; j++) {
                    TSNode grand_child = ts_node_child(child, j);
                    std::string gc_type(ts_node_type(grand_child));
                    
                    if (gc_type == "identifier") {
                        imported_symbols.push_back(getNodeText(grand_child, source_code));
                    } else if (gc_type == "named_imports") {
                        uint32_t gg_count = ts_node_child_count(grand_child);
                        for (uint32_t k = 0; k < gg_count; k++) {
                            TSNode import_specifier = ts_node_child(grand_child, k);
                            if (std::string(ts_node_type(import_specifier)) == "import_specifier") {
                                TSNode name_node = ts_node_child(import_specifier, 0);
                                imported_symbols.push_back(getNodeText(name_node, source_code));
                            }
                        }
                    }
                }
            }
        }
        for (const auto& sym : imported_symbols) {
            this->imports.push_back(std::make_pair(import_source, sym));
        }
    }
    // 2. 处理 Exports
    else if (type_str == "export_statement") {
        uint32_t child_count = ts_node_child_count(node);
        for (uint32_t i = 0; i < child_count; i++) {
            TSNode child = ts_node_child(node, i);
            std::string child_type(ts_node_type(child));

            if (child_type == "lexical_declaration" || child_type == "class_declaration" || child_type == "function_declaration") {
                // 【修复点 1】：加入参数 4
                TSNode id_node = ts_node_child_by_field_name(child, "name", 4);
                if (!ts_node_is_null(id_node)) {
                    this->exports.push_back(getNodeText(id_node, source_code));
                }
            } else if (child_type == "export_clause") {
                uint32_t export_spec_count = ts_node_child_count(child);
                for (uint32_t j = 0; j < export_spec_count; j++) {
                    TSNode spec = ts_node_child(child, j);
                    if (std::string(ts_node_type(spec)) == "export_specifier") {
                        TSNode name_node = ts_node_child(spec, 0);
                        this->exports.push_back(getNodeText(name_node, source_code));
                    }
                }
            }
        }
    }
    // 3. 处理 Objects
    else if (type_str == "class_declaration" || type_str == "function_declaration" || type_str == "interface_declaration") {
        // 【修复点 2】：加入参数 4
        TSNode id_node = ts_node_child_by_field_name(node, "name", 4);
        if (!ts_node_is_null(id_node)) {
            std::string obj_name = getNodeText(id_node, source_code);
            std::string obj_type = (type_str == "class_declaration") ? "class" : 
                                   (type_str == "function_declaration" ? "function" : "interface");
            
            ObjectNode* obj = new ObjectNode(obj_name, obj_type);
            
            TSPoint start_point = ts_node_start_point(node);
            TSPoint end_point = ts_node_end_point(node);
            obj->start_line = start_point.row + 1;
            obj->end_line = end_point.row + 1;

            this->objects_pool[obj_name] = obj;
        }
    }

    uint32_t child_count = ts_node_child_count(node);
    for (uint32_t i = 0; i < child_count; i++) {
        traverseAST(ts_node_child(node, i), source_code);
    }
}

void ModulesManager::FileNode::parse() {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "[Warning] Failed to open file for parsing: " << file_path << std::endl;
        return;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source_code = buffer.str();
    file.close();

    TSParser *parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_typescript());

    TSTree *tree = ts_parser_parse_string(parser, NULL, source_code.c_str(), source_code.length());
    if (!tree) {
        std::cerr << "[Error] Failed to parse source file AST: " << file_path << std::endl;
        ts_parser_delete(parser);
        return;
    }

    TSNode root_node = ts_tree_root_node(tree);
    traverseAST(root_node, source_code);

    ts_tree_delete(tree);
    ts_parser_delete(parser);
}