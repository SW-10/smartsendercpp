#include <iostream>
#include "models/Gorilla.h"
#include "doctest.h"
#include "managers/ConfigManager.h"
#include "managers/ReaderManager.h"
#include "arrow/api.h"
#include "ArrowFlight.h"

arrow::Status RunMain(){
    ConnectionAddress address("127.0.0.1", 9999);
    ARROW_ASSIGN_OR_RAISE(auto client, CreateClient(address));

}

int main(int argc, char *argv[]) {
    
   	//Doctest things
	doctest::Context context;
	int res = context.run();
	int client_stuff_return_code = 0;
    arrow::Status st = RunMain();
    if (!st.ok()) {
        std::cerr << st << std::endl;
        return 1;
    }

	return res + client_stuff_return_code;
    
}


