#include <iostream>
#include "models/gorilla.h"
#include "doctest.h"

int main() {
    
   	//Doctest things
	doctest::Context context;
	int res = context.run();
	int client_stuff_return_code = 0;

	return res + client_stuff_return_code;
    
}