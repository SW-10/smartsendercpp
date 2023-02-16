#include <iostream>
#include "models/gorilla.h"
#include "doctest.h"

int main() {
	doctest::Context context;

    std::cout << "hello there" << std::endl;
    Gorilla g;
    g = g.get_gorilla();
    g.fitValueGorilla(1.0);
    g.fitValueGorilla(1.3);
    g.fitValueGorilla(1.24);
    g.fitValueGorilla(1.045);
    g.fitValueGorilla(1.23);
    g.fitValueGorilla(1.54);
    g.fitValueGorilla(1.45);
    g.fitValueGorilla(1.12);
    g.fitValueGorilla(1.12);

    	//Doctest things
	int res = context.run();
	int client_stuff_return_code = 0;

	return res + client_stuff_return_code;
    
}