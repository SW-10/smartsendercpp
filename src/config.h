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
    int timestampCol = -1;
    int numberOfCols = 0;
    int number_of_text_cols = 0;
    std::string outPutCsvFile;
    int containsPosition = 0;
    char* output;
    explicit configParameters(std::string &path);
private:
    void column_or_text(int* count, char* token);
};



#endif //MOBYCPP_CONFIG_H
