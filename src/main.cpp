#include <iostream>
#include "models/Gorilla.h"
#include "doctest.h"
#include "managers/ConfigManager.h"
#include "managers/ReaderManager.h"

int main(int argc, char *argv[]) {
    
   	//Doctest things
	doctest::Context context;
	int res = context.run();
	int client_stuff_return_code = 0;
    std::string path = "moby.cfg";
    ReaderManager readerManager(path);
    readerManager.runCompressor();
	return res + client_stuff_return_code;
    
}