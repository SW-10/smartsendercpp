#include <iostream>
#include "models/Gorilla.h"
#include "doctest.h"
#include "managers/ReaderManager.h"
#include "arrow/api.h"
#include "ArrowFlight.h"

arrow::Status RunMain(){
    ConnectionAddress address("127.0.0.1", 9999);
    auto flight_client = CreateClient(address);

    auto sql_flight_client = CreateSQLClient(std::move(flight_client)).ValueOrDie();

    flight::FlightCallOptions call_options;

    std::cout << "Executing query: '" << "SELECT * FROM intTable WHERE value >= 0" << "'" << std::endl;
    ARROW_ASSIGN_OR_RAISE(std::unique_ptr<flight::FlightInfo> flight_info,
                          sql_flight_client->Execute(call_options, "SELECT * FROM intTable WHERE value >= 0"));

    return arrow::Status::OK();
}


int main() {
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


