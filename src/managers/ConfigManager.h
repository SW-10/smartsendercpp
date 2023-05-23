#pragma once

#include <string>
#include <cstring>
#include "vector"

struct columns {
    int col;
    double error;
    double maxError;
    double outlierThreshHold;

    columns(int col, double error, double outlierThreshHold, double maxError);
    columns() = default;
};

struct columnsExtra : columns{
    double errorSwing;
    columnsExtra(int col, double error, double outlierThreshHold, double maxError);
    columnsExtra() = default;
};

class ConfigManager {
public:
    int totalNumberOfCols = 0;
    int timestampCol = -1;
    int numberOfCols = 0;
    std::vector<columns> timeseriesCols;
    std::string inputFile;
    int maxAge = 0;
    int chunkSize = 0;
    int budget = 0;
    int bufferGoal = 0;
    int budgetLeftRegressionLength = 0;
    int chunksToGoal = 0;

    explicit ConfigManager(std::string &path);

private:

    char *output;

    void handleColumns(const int *count, char *token);

    void fixQuotation();
};