#include "ModulesManager.h"


int main(int argc, char **argv) {
	ModulesManager *mod_manager = new ModulesManager();
	mod_manager->scanProject("D:\\HarmonyOs\\Test2");
	delete mod_manager;
	return 0;
}