#include <iostream>
#include "models/gorilla.h"
#include "doctest.h"
#include "config.h"

int main(int argc, char *argv[]) {
    
   	//Doctest things
	doctest::Context context;
	int res = context.run();
	int client_stuff_return_code = 0;
    configParameters config = configParameters(argc, argv);

	return res + client_stuff_return_code;
    
}