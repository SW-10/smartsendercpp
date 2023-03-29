#pragma once

#include <string>
#include "arrow/flight/api.h"
#include "arrow/api.h"
#include "arrow/flight/sql/api.h"
#include "managers/ModelManager.h"

namespace flight = arrow::flight;
namespace flightsql = arrow::flight::sql;

struct ConnectionAddress {
    ConnectionAddress(std::string host, int32_t port);

    std::string host;
    int32_t port;
};

arrow::Result<std::shared_ptr<arrow::Table>> VectorToColumnarTable(
        const std::vector<struct SelectedModel>& rows);

arrow::Result<std::shared_ptr<arrow::RecordBatch>> MakeRecordBatch(const
                                                                   std::shared_ptr<arrow::Table>& table);

arrow::Status WriteRecordBatchVectorToParquet(
        std::shared_ptr<arrow::Table> &table,
        std::string pathToFile);

arrow::Result<std::unique_ptr<flight::FlightClient>>
createClient(const ConnectionAddress &address);

arrow::Result<std::unique_ptr<flightsql::FlightSqlClient>>
createSqlClient(std::unique_ptr<flight::FlightClient> flightClient);
