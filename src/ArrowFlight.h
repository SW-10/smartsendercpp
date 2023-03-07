#include <string>
#include "arrow/flight/api.h"
#include "arrow/api.h"
#include "arrow/flight/sql/api.h"

namespace flight = arrow::flight;
namespace flightsql = arrow::flight::sql;

struct ConnectionAddress {
    ConnectionAddress(std::string host, int32_t port);

    std::string host;
    int32_t port;
};

arrow::Result<std::unique_ptr<flight::FlightClient>> CreateClient(const ConnectionAddress &address);
arrow::Result<std::unique_ptr<flightsql::FlightSqlClient>> CreateSQLClient(auto flightClient);
