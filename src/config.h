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
    float error;
    int isAbsolute;
};

class configParameters {
public:

    columns latCol, longCol;
    std::vector<columns> cols;
    std::vector<int> text_cols;
    int timestampCol;
    int numberOfCols = 0;
    int number_of_text_cols;
    std::string outPutCsvFile;
    int containsPosition;
    char* output;
    configParameters(int argc, char *argv[]);
    void column_or_text(int* count, char* token);
};



#endif //MOBYCPP_CONFIG_H
