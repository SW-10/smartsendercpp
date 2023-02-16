#include <iostream>
#include "models/gorilla.h"
#include "models/PMCMean.h"
#include "models/test.h"
#include "doctest.h"

int main() {
    Pmc_mean pmc;
    pmc.fit_value_pmc(1, 1);
   	//Doctest things
	doctest::Context context;
	int res = context.run();
	int client_stuff_return_code = 0;

    test t;
    t.printhello();

    pmc.grid_pmc_mean(10, 5);
	return res + client_stuff_return_code;
}