
#include <iostream>
#include "arrow/flight/api.h"
#include "arrow/api.h"
#include "arrow/flight/sql/api.h"
#include "ArrowFlight.h"
#include "managers/ModelManager.h"

namespace flight = arrow::flight;
namespace flightsql = arrow::flight::sql;

/*arrow::Status MakeArray() {
    arrow::Int8Builder int8builder;
    std::vector<int8_t> mid = {PMC_MEAN,SWING,GORILLA};
    ARROW_RETURN_NOT_OK(int8builder.AppendValues(mid, 5));

    std::unique_ptr<arrow::flight::FlightClient> client;


}
ConnectionAddress::ConnectionAddress(std::string hostname, int32_t port) {
    this->host = hostname;
    this->port = port;
};
*/


arrow::Result<std::unique_ptr<flight::FlightClient>> CreateClient(ConnectionAddress address) {
    ARROW_ASSIGN_OR_RAISE(auto location,
                          arrow::flight::Location::ForGrpcTcp(address.host, address.port));
    arrow::flight::FlightServerOptions options(location);

    ARROW_ASSIGN_OR_RAISE(auto flight_client, flight::FlightClient::Connect(location));
    std::cout << "Connected to server: localhost:" << address.port << std::endl;
    return flight_client;
}


arrow::Result<std::unique_ptr<flightsql::FlightSqlClient>>
CreateSQLClient(std::unique_ptr<flight::FlightClient> flightClient) {
    std::unique_ptr<flightsql::FlightSqlClient> sql_client(
            new flightsql::FlightSqlClient(std::move(flightClient)));
    std::cout << "SQL Client created." << std::endl;
    return sql_client;
}
