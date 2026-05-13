#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>

extern "C" {
    #include "tree_sitter/api.h"
    const TSLanguage *tree_sitter_typescript();
}

class ArkTSAnalyzer {
private:
    TSParser* parser_;
    TSQuery* query_;

    // 读取文件内容的辅助函数
    std::string readFile(const std::string& file_path) {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            std::cerr << "警告: 无法打开文件 " << file_path << std::endl;
            return "";
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

public:
    // 构造函数：初始化引擎和查询语句（只执行一次，极大提升性能）
    ArkTSAnalyzer() {
        parser_ = ts_parser_new();
        ts_parser_set_language(parser_, tree_sitter_typescript());

        std::string query_str = R"(
            (export_statement declaration: (class_declaration name: (identifier) @export.class))
            (export_statement declaration: (function_declaration name: (identifier) @export.function))
        )";

        uint32_t error_offset;
        TSQueryError error_type;
        query_ = ts_query_new(
            tree_sitter_typescript(),
            query_str.c_str(),
            query_str.length(),
            &error_offset,
            &error_type
        );

        if (error_type != TSQueryErrorNone) {
            throw std::runtime_error("Tree-sitter 查询语句编译失败！");
        }
    }

    // 析构函数：自动清理 C 语言分配的内存
    ~ArkTSAnalyzer() {
        if (query_) ts_query_delete(query_);
        if (parser_) ts_parser_delete(parser_);
    }

    
};