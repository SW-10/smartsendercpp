#include <iostream>
#include "models/Gorilla.h"
#include "doctest.h"
#include "managers/ReaderManager.h"
#include "arrow/api.h"
#include "ArrowFlight.h"

arrow::Status runMain(){
    ConnectionAddress address("127.0.0.1", 9999);
    auto flightClient = createClient(address);

    auto sqlFlightClient = createSqlClient(flightClient.ValueOrDie());

    flight::FlightCallOptions call_options;

    std::cout << "Executing query: '" << "SELECT * FROM intTable WHERE value >= 0" << "'" << std::endl;
    ARROW_ASSIGN_OR_RAISE(std::unique_ptr<flight::FlightInfo> flightInfo,
                          sqlFlightClient->Execute(call_options, "SELECT * FROM intTable WHERE value >= 0"));

    return arrow::Status::OK();
}


int main() {
    //Doctest things
    doctest::Context context;
    int res = context.run();
    int client_stuff_return_code = 0;
    arrow::Status st = runMain();
    if (!st.ok()) {
        std::cerr << st << std::endl;
        return 1;
    }

    return res + client_stuff_return_code;
}


