//
// Created by power on 15-02-2023.
//


#ifndef MOBYCPP_CONFIG_H
#define MOBYCPP_CONFIG_H

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
    explicit ConfigManager(std::string &path);
    columns* getLatColumn() { return &latCol; }
    columns* getLongColumn() { return &longCol; }
    std::vector<columns>* getTimeSeriesColumns() { return &cols;}
    int getTimestampColumn() { return timestampCol;}
    bool getContainsPosition() { return containsPosition;}
    std::string getInputFile() { return "../" + inputFile;}
    std::vector<int>* getTextColumns() { return &text_cols; }
    int getNumberOfCols() { return numberOfCols; }


private:
    void column_or_text(int* count, char* token);
    columns latCol, longCol;
    std::vector<columns> cols;
    std::vector<int> text_cols;
    int timestampCol = -1;
    int numberOfCols = 0;
    int number_of_text_cols = 0;
    std::string outPutCsvFile;
    bool containsPosition = false;
    std::string inputFile;
    //Future use for MQTT credentials
    char* output;
};



#endif //MOBYCPP_CONFIG_H
