#include <fstream>
#include <filesystem>
#include "ConfigManager.h"
#include "../getopt.h"
#include <algorithm>
#include "memory"

ConfigManager::ConfigManager(std::string &path) {
    int c;
    int digit_optind = 0;
    const char s[2] = " ";
    char *token;
    int count = 0;
    std::ifstream configFile;
    configFile.open("../" + path);

    //Read configfile into vector
    std::vector<std::string> innerCharVector;
    if (configFile.is_open()) {
        while (configFile) {
            std::string tpTemp;
            std::getline(configFile, tpTemp);
            innerCharVector.emplace_back(tpTemp);
        }
    }

    //Convert strings to char* vector
    std::vector<char *> outerCharVector;
    for (std::string &v: innerCharVector) {
        outerCharVector.emplace_back(v.data());
        if (v.empty()) {
            outerCharVector.emplace_back(v.data());
        }
    }

    //Rotate vector to get empty char* in element 0, as getopt need
    rotate(outerCharVector.rbegin(), outerCharVector.rbegin() + 1,
           outerCharVector.rend());
    char **argsEmulator = outerCharVector.data();

    while (true) {
        int this_option_optind = optind ? optind : 1;
        int option_index = 0;
        static struct option long_options[] = {
                {"columns",    required_argument, 0, 'c'},
                {"timestamps", required_argument, 0, 't'},
                {"output",     required_argument, 0, 'o'},
                {"text",       required_argument, 0, 'x'},
                {"inputFile",  required_argument, 0, 'i'},
                {"maxAge",     required_argument, 0, 'm'},
                {"chunkSize",  required_argument, 0, 's'},
                {"budget",     required_argument, 0, 'b'},
                {"bufferGoal", required_argument, 0, 'g'},
                {"budgetLeftRegressionLength", required_argument, 0, 'r'},
                {"chunksToGoal", required_argument, 0, 'l'},
                {0,            0                , 0,  0 }
        };

        c = getopt_long(outerCharVector.size(), argsEmulator, "p:c:t:o:x:",
                        long_options, &option_index);
        if (c == -1) break;
        switch (c) {
            case 'l':
                fixQuotation();
                this->chunksToGoal = atoi(optarg);
                break;
            case 'r':
                fixQuotation();
                this->budgetLeftRegressionLength = atoi(optarg);
                break;
            case 'g':
                fixQuotation();
                this->bufferGoal = atoi(optarg);
                break;
            case 'b':
                fixQuotation();
                this->budget = atoi(optarg);
                break;
            case 'm':
                fixQuotation();
                this->maxAge = atoi(optarg);
                break;
            case 's':
                if(optarg[strlen(optarg)-1] == '\r'){
                    optarg[strlen(optarg)-1] = '\0';
                }
                if(optarg[0] == '\'' || optarg[0] == '\"'){
                    optarg = &optarg[1];
                }
                if (optarg[strlen(optarg) - 1] == '\'' ||
                    optarg[strlen(optarg) - 1] == '\"') {
                    optarg[strlen(optarg) - 1] = '\0';
                }
                this->chunkSize = atoi(optarg);
                break;
            case 'c':
                //From documentation. Not sure what it does
                if (digit_optind != 0 && digit_optind != this_option_optind)
                    //printf("digits occur in two different argv-elements.\n");
                    digit_optind = this_option_optind;

                // Debug mode seems to add single quotation marks around the arguments.
                // The following two if's remove those
                if (optarg[0] == '\'' || optarg[0] == '\"') {
                    optarg = &optarg[1];
                }
                if (optarg[strlen(optarg) - 1] == '\'' ||
                    optarg[strlen(optarg) - 1] == '\"') {
                    optarg[strlen(optarg) - 1] = '\0';
                }

                count = 0;
                token = strtok(optarg, s);
                while (token != NULL) {
                    columnOrText(&count, token);
                    token = strtok(NULL, s); // NULL is not a mistake!
                    count++;
                }

                if (count % 3 != 0) {
                    printf("Not the expected number of arguments for columns. Number of parameters should be divisible by 3 and follow the following format:\n");
                    printf("<column (int)> <error (float)> - <max error (float) <Outlier thresh hold (float)>\n");
                }

                totalNumberOfCols += (count / 3);
                break;
            case 't':
                if (optarg[0] == '\'' || optarg[0] == '\"') {
                    optarg = &optarg[1];
                }
                if (optarg[strlen(optarg) - 1] == '\'' ||
                    optarg[strlen(optarg) - 1] == '\"') {
                    optarg[strlen(optarg) - 1] = '\0';
                }
                timestampCol = atoi(optarg) - 1;
                totalNumberOfCols++;
                break;
            case 'o':
                //Future use for flight credentials
                output = optarg;
                outPutCsvFile = optarg;
                outPutCsvFile += "/";
                break;
            case 'i':
                if(optarg[strlen(optarg)-1] == '\r'){
                    optarg[strlen(optarg)-1] = '\0';
                }
                if(optarg[0] == '\'' || optarg[0] == '\"'){
                    optarg = &optarg[1];
                }
                if (optarg[strlen(optarg) - 1] == '\'' ||
                    optarg[strlen(optarg) - 1] == '\"') {
                    optarg[strlen(optarg) - 1] = '\0';
                }
                this->inputFile = optarg;
                break;
            default:
                printf("Unknown option, exiting ...\n");
                exit(1);
        }
    }

    if (timestampCol < 0) {
        printf("Timestamp column must be specified, and it must be above 0. It should follow the following format:\n");
        printf("--timestamps <column (int)>\n");
        exit(1);
    }
}

void ConfigManager::columnOrText(const int *count, char *token) {
    // Handle arg here
    if (*count % 3 == 0) {
        numberOfCols++;
        columns &ptr = timeseriesCols.emplace_back();
        ptr.col = atoi(token) - 1;
    }
    if (*count % 3 == 1) {
        columns &ptr = timeseriesCols.back();
        std::string errors = token;
        size_t index = errors.find('-');
        ptr.error = std::stod(errors.substr(0,index));
        ptr.maxError = std::stod(errors.substr(index+1, errors.length()));
    }
    if (*count % 3 == 2) {
        columns &ptr = timeseriesCols.back();
        ptr.outlierThreshHold = atof(token);
    }
}

void ConfigManager::fixQuotation(){
    // Debug mode seems to add single quotation marks around the arguments.
    // The following two if's remove those
    if(optarg[strlen(optarg)-1] == '\r'){
        optarg[strlen(optarg)-1] = '\0';
    }
    if(optarg[0] == '\'' || optarg[0] == '\"'){
        optarg = &optarg[1];
    }
    if (optarg[strlen(optarg) - 1] == '\'' ||
        optarg[strlen(optarg) - 1] == '\"') {
        optarg[strlen(optarg) - 1] = '\0';
    }
}


columns::columns(int col, double error, double outlierThreshHold, double maxError) {
    this->col = col;
    this->error = error;
    this->outlierThreshHold = outlierThreshHold;
    this->maxError = maxError;
}

columnsExtra::columnsExtra(int col, double error, double outlierThreshHold, double maxError) : columns(col, error, outlierThreshHold, maxError) {
    this->errorSwing = error;
}
