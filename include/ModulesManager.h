#pragma once

#include <iostream>
#include <string>
#include <map>
#include <filesystem>
#include <fstream>
#include <vector>
#include <stack>
#include <utility>
#include "nlohmann/json.hpp"
#include "FileAnalyzer.h"

class ModulesManager {
private:
	/*
		函数依赖于文件
		文件之间存在依赖关系
		跨模块的部分，我们只关心 main_file 给出的接口
	*/
	struct ObjectNode {
		ObjectNode(const std::string &_name, const std::string &_type) : name(_name), type(_type) {}
		std::string name;
		// function / class
		std::string type;
		std::vector<std::string> calls; 
		std::vector<std::string> called_by;
	};
	struct FileNode {
		FileNode(const std::string &_path) : file_path(_path) {};
		~FileNode() {
			for (auto it : objects_pool) {
				delete it.second;
			}
		}

		void parse() {
			
		}

		std::string file_path, module_path;
		// 文件依赖关系 (object, file)
		std::vector<std::pair<std::string, std::string>> imports;
		std::vector<std::pair<std::string, std::string>> imported_by;
		// 暴露的接口
		std::vector<std::string> exports; 
		std::map<std::string, ObjectNode*> objects_pool; 
	};
	struct ModuleNode {
		ModuleNode(const std::string &_path) : path(_path) {};
		std::string path;
		std::vector<std::string> depends;
		std::vector<std::string> depended_by;
		std::map<std::string, std::string> dependname_to_path;
		FileNode *main_file = nullptr;
		void dependOn(ModuleNode *rhs) {
			if (rhs == nullptr) return;
			rhs->depended_by.push_back(path);
			this->depends.push_back(rhs->path);
		}
		void setMainFile(const std::string &file_path) {
			main_file = new FileNode(file_path);
		}
		// code path: ./src/main/ets
		// oh-package.json5: ./
	};
public:
	ModulesManager() {};
	~ModulesManager() {
		for (auto it : modules_pool) {
			delete it.second;
		}
	};
public:
	void scanProject(const std::string &root_path) {
		namespace fs = std::filesystem;
		for (auto it = fs::recursive_directory_iterator(root_path);
			it != fs::end(it); ++it
		) {
			const auto &entry = *it;
			const auto &path = entry.path();
			if (path.filename().string() == "oh_modules" || path.filename().string().find(".") == 0) {
				it.disable_recursion_pending();
				continue;
			}
			if (isModule(path.string())) {
				parseModule(path.string());
			}
		}
	}
	void scanModule(const std::string &root_path) {
		namespace fs = std::filesystem;
		for (auto it = fs::recursive_directory_iterator(root_path);
			it != fs::end(it); ++it
		) {
			const auto &entry = *it;
			const auto &path = entry.path();
			if (path.filename().string() == "oh_modules" || path.filename().string().find(".") == 0) {
				it.disable_recursion_pending();
				continue;
			}
			if (isModule(path.string())) {
				it.disable_recursion_pending();
				continue;
			}
			if (path.extension() == "ets") {
				FileNode *cur_file = getFile(path.string());
				cur_file->module_path = root_path;
				cur_file->parse();
			}
		}
	}
private:
	bool isModule(const std::string &folder_path) {
		std::filesystem::path p(folder_path);
		if (!std::filesystem::exists(p) || !std::filesystem::is_directory(folder_path)) {
			return false;
		}
		if (std::filesystem::exists(p / "oh-package.json5")) {
			return true;
		}
		return false;
	}
	ModuleNode* getModule(const std::string &path) {
		if (modules_pool.find(path) == modules_pool.end()) {
			modules_pool.insert({path, new ModuleNode(path)});
		}
		return modules_pool[path];
	}
	void parseModulePackage(const std::string& file_path) {
		using json = nlohmann::json;
		std::ifstream f(file_path + "/oh-package.json5");
		if (!f.is_open()) return;
		json data = json::parse(f);
		ModuleNode *mod = getModule(file_path);
		if (mod == nullptr) return;
		if (data.contains("dependencies")) {
			auto deps = data["dependencies"];
			for (auto it = deps.begin(); it != deps.end(); ++it) {
				std::string dep_path = it.value();
				if (dep_path.find("file:") == 0) {
					mod->dependOn(getModule(PathAnalyzer::getPathFromTo(file_path, dep_path.substr(5))));
				}
			}
		}
		if (data.contains("main") && !data["main"].empty()) {
			mod->setMainFile(PathAnalyzer::getPathFromTo(file_path, data["main"]));
		}
	}
	void parseModule(const std::string &file_path) {
		parseModulePackage(file_path);
		scanModule(file_path);
	}
	FileNode* getFile(const std::string &path) {
		if (files_pool.find(path) == files_pool.end()) {
			files_pool.insert({path, new FileNode(path)});
		}
		return files_pool[path];
	}
private:
	std::map<std::string, FileNode*> files_pool; // 路径 -> 文件
	std::map<std::string, ModuleNode*> modules_pool; // 路径 -> 模块
};

