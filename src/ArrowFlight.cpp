
#include <iostream>
#include "arrow/flight/api.h"
#include "arrow/api.h"
#include "arrow/flight/sql/api.h"
#include "ArrowFlight.h"
#include "managers/ModelManager.h"

namespace flight = arrow::flight;
namespace flightsql = arrow::flight::sql;

std::shared_ptr<arrow::Array> MakeInt8Array(std::vector<uint8_t> input) {
    arrow::Int8Builder builder;
    for(uint8_t v : input){
        builder.Append(v);
    }
    auto array = builder.Finish().ValueOrDie();

    return array;
}

std::shared_ptr<arrow::Array> MakeFloat32Array(std::vector<float> input) {
    arrow::FloatBuilder builder;
    for(float v : input){
        builder.Append(v);
    }
    auto array = builder.Finish().ValueOrDie();

    return array;
}

std::shared_ptr<arrow::Array> MakeTimestampArray(std::vector<uint64_t> input) {
    arrow::TimestampBuilder(arrow::TimestampType(arrow::TimeUnit::MILLI, "")) builder;
    for(uint64_t v : input){
        builder.Append(v);
    }
    auto array = builder.Finish().ValueOrDie();

    return array;
}


arrow::Status MakeRecordBatch(){
    auto schema = arrow::schema({arrow::field("mid", arrow::int8()), arrow::field("cid", arrow::int8()), arrow::field("value", arrow::float32()), arrow::field("timestamp", arrow::timestamp(arrow::TimeUnit::MILLI))});
}


ConnectionAddress::ConnectionAddress(std::string host, int32_t port) {
    this->host = host;
    this->port = port;
};

arrow::Result<std::unique_ptr<flight::FlightClient>> CreateClient(ConnectionAddress &address) {
    ARROW_ASSIGN_OR_RAISE(auto location,
                          arrow::flight::Location::ForGrpcTcp(address.host, address.port));
    arrow::flight::FlightServerOptions options(location);

    ARROW_ASSIGN_OR_RAISE(auto flight_client, flight::FlightClient::Connect(location));
    std::cout << "Connected to server:" << address.host << ":" << address.port << std::endl;
    return flight_client;
}


arrow::Result<std::unique_ptr<flightsql::FlightSqlClient>>
CreateSQLClient(std::unique_ptr<flight::FlightClient> flightClient) {
    std::unique_ptr<flightsql::FlightSqlClient> sql_client(
            new flightsql::FlightSqlClient(std::move(flightClient)));
    std::cout << "SQL Client created." << std::endl;
    return sql_client;
}
