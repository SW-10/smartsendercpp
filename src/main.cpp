#include <iostream>
#include "models/gorilla.h"
#include "models/PMCMean.h"
#include "doctest.h"
#include "models/swing.h"

int main() {
    Pmc_mean pmc;
    pmc.fit_value_pmc(1, 1);
   	//Doctest things
	doctest::Context context;
	int res = context.run();
	int client_stuff_return_code = 0;
    pmc.grid_pmc_mean(10, 5);

    Swing s;
    fitValueSwing(&s, 1, 1.4, 1);


	return res + client_stuff_return_code;

    
}