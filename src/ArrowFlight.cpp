
#include <iostream>
#include "arrow/flight/api.h"
#include "arrow/api.h"
#include "arrow/flight/sql/api.h"
#include "ArrowFlight.h"
#include "managers/ModelManager.h"

namespace flight = arrow::flight;
namespace flightsql = arrow::flight::sql;


arrow::Result<std::shared_ptr<arrow::Table>>
VectorToColumnarTable(const std::vector<struct SelectedModel> &rows) {
    arrow::MemoryPool *pool = arrow::default_memory_pool();

    arrow::Int8Builder midBuilder(pool);
    arrow::Int8Builder cidBuilder(pool);
    arrow::TimestampBuilder startTimeBuilder(arrow::timestamp
                                                     (arrow::TimeUnit::MILLI,
                                                      ""), pool);
    arrow::TimestampBuilder endTimeBuilder(arrow::timestamp
                                                   (arrow::TimeUnit::MILLI,
                                                    ""), pool);

    arrow::ListBuilder valuesBuilder(pool, std::make_shared<arrow::FloatBuilder>
            (pool));
    // The following builder is owned by valuesBuilder.
    arrow::FloatBuilder *valuesItemBuilder =
            (dynamic_cast<arrow::FloatBuilder *>(valuesBuilder
                    .value_builder()));
    arrow::FloatBuilder errorBuilder(pool);

    for (const SelectedModel &row: rows) {
        ARROW_RETURN_NOT_OK(midBuilder.Append(row.mid));
        ARROW_RETURN_NOT_OK(cidBuilder.Append(row.cid));
        ARROW_RETURN_NOT_OK(startTimeBuilder.Append(row.startTime));
        ARROW_RETURN_NOT_OK(endTimeBuilder.Append(row.endTime));

        ARROW_RETURN_NOT_OK(valuesBuilder.Append());
        ARROW_RETURN_NOT_OK(valuesItemBuilder->AppendValues(
                row.values.data(), row.values.size()));
        ARROW_RETURN_NOT_OK(errorBuilder.Append(row.error));
    }

    std::shared_ptr<arrow::Array> midArray;
    ARROW_RETURN_NOT_OK(midBuilder.Finish(&midArray));
    std::shared_ptr<arrow::Array> cidArray;
    ARROW_RETURN_NOT_OK(cidBuilder.Finish(&cidArray));
    std::shared_ptr<arrow::Array> startTimeArray;
    ARROW_RETURN_NOT_OK(startTimeBuilder.Finish(&startTimeArray));
    std::shared_ptr<arrow::Array> endTimeArray;
    ARROW_RETURN_NOT_OK(endTimeBuilder.Finish(&endTimeArray));
    std::shared_ptr<arrow::Array> valuesArray;
    ARROW_RETURN_NOT_OK(valuesBuilder.Finish(&valuesArray));
    std::shared_ptr<arrow::Array> errorArray;
    ARROW_RETURN_NOT_OK(errorBuilder.Finish(&errorArray));

    std::vector<std::shared_ptr<arrow::Field>> schemaVector = {
            arrow::field("mid", arrow::int8()),
            arrow::field("cid", arrow::int8()),
            arrow::field
                    ("startTime",
                     arrow::timestamp
                             (arrow::TimeUnit::MILLI)),
            arrow::field
                    ("endTime",
                     arrow::timestamp(
                             arrow::TimeUnit::MILLI)),
            arrow::field("values", arrow::list(arrow::float32())),
            arrow::field("error", arrow::float32())};

    auto schema = std::make_shared<arrow::Schema>(schemaVector);

    std::shared_ptr<arrow::Table> table =
            arrow::Table::Make(schema, {midArray,cidArray,
                                        startTimeArray, endTimeArray,
                                        valuesArray, errorArray});

    return table;
}


arrow::Result<std::shared_ptr<arrow::RecordBatch>>
MakeRecordBatch(const std::shared_ptr<arrow::Table> &table) {

    std::shared_ptr<arrow::RecordBatch> rBatch;
    ARROW_ASSIGN_OR_RAISE(rBatch, table->CombineChunksToBatch
            (arrow::default_memory_pool()))

    return rBatch;
}


ConnectionAddress::ConnectionAddress(std::string host, int32_t port) {
    this->host = std::move(host);
    this->port = port;
}

arrow::Result<std::unique_ptr<flight::FlightClient>>
createClient(const ConnectionAddress &address) {
    ARROW_ASSIGN_OR_RAISE(auto location,
                          arrow::flight::Location::ForGrpcTcp(address.host,
                                                              address.port))
    arrow::flight::FlightServerOptions options(location);

    ARROW_ASSIGN_OR_RAISE(auto flight_client,
                          flight::FlightClient::Connect(location))
    std::cout << "Connected to server:" << address.host << ":" << address.port
              << std::endl;
    return flight_client;
}


arrow::Result<std::unique_ptr<flightsql::FlightSqlClient>>
createSqlClient(std::unique_ptr<flight::FlightClient> flightClient) {
    std::unique_ptr<flightsql::FlightSqlClient> sql_client(
            new flightsql::FlightSqlClient(std::move(flightClient)));
    std::cout << "SQL Client created." << std::endl;
    return sql_client;
}
