#pragma once

#include <iostream>
#include <string>
#include <map>
#include <filesystem>
#include <fstream>
#include <vector>
#include "nlohmann/json.hpp"



class ModulesManager {
private:
	/*
		以文件作为单位分析，
	*/
	struct FunctionNode {
		FunctionNode(const std::string &_name) : name(_name) {}
		std::string name;
		std::vector<std::string> calls; 
		std::vector<std::string> called_by;
	};
	struct FileNode {
		FileNode(const std::string &_path) : file_path(_path) {};
		~FileNode() {
			for (auto it : functions_pool) {
				delete it.second;
			}
		}
		std::string file_path;
		// 文件依赖关系
		std::vector<std::string> imports;
		std::vector<std::string> imported_by;
		// 暴露的函数接口
		std::vector<std::string> exports; 
		std::map<std::string, FunctionNode*> functions_pool; 
	};
	struct ModuleNode {
		ModuleNode(const std::string &_name) : name(name) {};
		std::string name;
		std::vector<std::string> depends;
		std::vector<std::string> depended_by;
		void dependOn(ModuleNode *rhs) {
			if (rhs == nullptr) return;
			rhs->depended_by.push_back(name);
			this->depends.push_back(rhs->name);
		}
		// code path: ./src/main/ets
		// oh-package.json5: ./
	};
private:
	ModulesManager() {};
	~ModulesManager() {
		for (auto it : modules_pool) {
			delete it.second;
		}
	};
public:
	static ModulesManager* getInstance() {
		if (instance == nullptr) {
			instance = new ModulesManager();
		}
		return instance;
	}
	void scanProject(const std::string &rootpath) {
		namespace fs = std::filesystem;
		for (auto it = fs::recursive_directory_iterator(rootpath);
			it != fs::end(it); ++it
		) {
			const auto &entry = *it;
			const auto &path = entry.path();
			if (path.filename().string() == "oh_modules" || path.filename().string().find(".") == 0) {
				it.disable_recursion_pending();
				continue;
			}
			if (isModule(path.string())) {
				// std::cout << "find a module: " << path << std::endl; 
				parseModule(path.string());
			}
		}
	}
private:
	bool isModule(const std::string &folderpath) {
		std::filesystem::path p(folderpath);
		if (!std::filesystem::exists(p) || !std::filesystem::is_directory(folderpath)) {
			return false;
		}
		if (std::filesystem::exists(p / "oh-package.json5")) {
			return true;
		}
		return false;
	}
	ModuleNode* getModule(const std::string &name) {
		if (modules_pool.find(name) == modules_pool.end()) {
			modules_pool.insert({name, new ModuleNode(name)});
		}
		return modules_pool[name];
	}
	void parseModulePackage(const std::string& file_path) {
		using json = nlohmann::json;
		std::ifstream f(file_path);
		if (!f.is_open()) return;
		json data = json::parse(f);
		std::string name = data["name"];
		// std::cout << "current module: " << name << std::endl;s
		ModuleNode *mod = getModule(name);
		if (mod == nullptr) return;
		if (data.contains("dependencies")) {
			auto deps = data["dependencies"];
			// std::cout << "find dependencies:" << std::endl;
			for (auto it = deps.begin(); it != deps.end(); ++it) {
				std::string dep_path = it.value();
				if (dep_path.find("file:") == 0) {
					// std::cout << "\t-> " << it.key() << ": " << dep_path.substr(5) << std::endl;
					mod->dependOn(getModule(it.key()));
				}
			}
		}
	}
	void parseModule(const std::string &file_path) {
		parseModulePackage(file_path + "/oh-package.json5");
	}
private:
	static ModulesManager* instance;
	std::map<std::string, ModuleNode*> modules_pool;
};

