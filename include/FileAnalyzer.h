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

class PathAnalyzer {
public:
    static std::string getPathFromTo(const std::string &now, const std::string &tar) {
        std::filesystem::path target_path(tar);
        if (target_path.is_relative()) {
            target_path = std::filesystem::path(now) / target_path;
            target_path = std::filesystem::weakly_canonical(target_path);
        } else {
            // 不符合规范，扣分
        }
        return target_path.string();
    }
private:
    PathAnalyzer() = delete;
    ~PathAnalyzer() = delete;
};
