#pragma once

#include <string>
#include <cstring>
#include "vector"

struct columns {
    int col;
    double error;
    int isAbsolute;
};

class ConfigManager {
public:
    columns latCol, longCol;
    int totalNumberOfCols = 0;
    int timestampCol = -1;
    int numberOfCols = 0;
    bool containsPosition = false;
    std::vector<columns> timeseriesCols;
    std::vector<int> textCols;
    std::string inputFile;
    int maxAge = 0;
    int chunkSize = 0;
    int budget = 0;

    explicit ConfigManager(std::string &path);

private:
    int number_of_text_cols = 0;
    std::string outPutCsvFile;

    //Future use for MQTT credentials
    char *output;

    void columnOrText(int *count, char *token);

    void adjustErrorBound(int globId, double errorBound);
};