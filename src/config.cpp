//
// Created by power on 15-02-2023.
//

#include <fstream>
#include <filesystem>
#include "config.h"
#include "getopt.h"
#include "memory"



configParameters::configParameters(std::string &path){
    int c;
    int digit_optind = 0;
    const char s[2] = " ";
    char *token;
    int count = 0;
    std::ifstream configFile;
    configFile.open("../"+path);

    //Read configfile into vector
    std::vector<std::string> innerCharVector;
    if ( configFile.is_open() ) {
        while ( configFile ) {
            std::string tpTemp;
            std::getline (configFile, tpTemp);
            innerCharVector.emplace_back(tpTemp);
        }
    }

    //Convert strings to char* vector
    std::vector<char *> outerCharVector;
    for (std::string &v : innerCharVector){
        outerCharVector.emplace_back(v.data());
        if (v.empty()){
            outerCharVector.emplace_back(v.data());
        }
    }
    
    //Rotate vector to get empty char* in element 0, as getopt need
    std::rotate(outerCharVector.rbegin(), outerCharVector.rbegin() + 1, outerCharVector.rend());
    char** argsEmulator = outerCharVector.data();

    while (true)
    {
        int this_option_optind = optind ? optind : 1;
        int option_index = 0;
        static struct option long_options[] = {
                {"position", required_argument, 0, 'p'},
                {"columns", required_argument, 0, 'c'},
                {"timestamps", required_argument, 0, 't'},
                {"output", required_argument, 0, 'o'},
                {"text", required_argument, 0, 'x'},
                {0, 0, 0, 0}};

        c = getopt_long(outerCharVector.size(), argsEmulator, "p:c:t:o:x:",
                        long_options, &option_index);
        if (c == -1)
            break;
        // printf("DEBUG: %d\n", debug);
        //innerCharVector;
        switch (c)
        {
            case 'p':
                // Debug mode seems to add single quotation marks around the arguments.
                // The following two if's remove those

                if(optarg[0] == '\'' || optarg[0] == '\"'){
                    optarg = &optarg[1];
                }
                if(optarg[strlen(optarg)-1] == '\'' || optarg[strlen(optarg)-1] == '\"'){
                    optarg[strlen(optarg)-1] = '\0';
                }

                count = 0;
                token = strtok(optarg, s);
                while( token != NULL ) {
                    count++;
                    // Handle args here
                    //printf( " %s\n", token );
                    if(count==1) latCol.col = atoi(token);
                    if(count==2) longCol.col = atoi(token);
                    if(count==3){
                        latCol.error  = atof(token);
                        longCol.error = atof(token);
                        containsPosition = true;
                    }

                    if(count>3){
                        printf("Too many arguments for position. Arguments should be: <lat> <long> <error>\n");
                        exit(1);
                    }

                    // Get next arg
                    token = strtok(NULL, s); // NULL is not a mistake!
                }

                if(count<3){
                    printf("Too few arguments for position. Arguments should be: <lat> <long> <error>\n");
                    exit(1);
                }

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
                if (optarg[strlen(optarg) - 1] == '\'' || optarg[strlen(optarg) - 1] == '\"') {
                    optarg[strlen(optarg) - 1] = '\0';
                }

                count = 0;

                token = strtok(optarg, s);
                while (token != NULL) {
                    column_or_text(&count, token);

                    token = strtok(NULL, s); // NULL is not a mistake!
                    count++;
                }

                if (count % 3 != 0) {
                    printf("Not the expected number of arguments for columns. Number of parameters should be divisible by 3 and follow the following format:\n");
                    printf("<column (int)> <error (float)> <absolute (A) / relative (R)>\n");
                    //exit(1);
                }
                break;
            case 'x':
                //From documentation. Not sure what it does
                if (digit_optind != 0 && digit_optind != this_option_optind)
                    printf("digits occur in two different argv-elements.\n");
                digit_optind = this_option_optind;

                // Debug mode seems to add single quotation marks around the arguments.
                // The following two if's remove those
                if (optarg[0] == '\'' || optarg[0] == '\"') {
                    optarg = &optarg[1];
                }
                if (optarg[strlen(optarg) - 1] == '\'' || optarg[strlen(optarg) - 1] == '\"') {
                    optarg[strlen(optarg) - 1] = '\0';
                }

                count = 0;

                token = strtok(optarg, s);
                while (token != NULL) {

                        // argStruct.cols->currentSize++;
                        text_cols.emplace_back(atoi(token));
                        number_of_text_cols++;
                        //printf("SIZE: %d\n", sizeof(*argStruct.text_cols) * argStruct.number_of_text_cols);
                        //text_cols = realloc(text_cols, sizeof(*text_cols) * number_of_text_cols);
                        //text_cols[number_of_text_cols-1] = atoi(token);

                    //printf("Column: %s\n", token);

                    token = strtok(NULL, s); // NULL is not a mistake!
                    count++;
                }
                break;
            case 't':
                if(optarg[0] == '\'' || optarg[0] == '\"'){
                    optarg = &optarg[1];
                }
                if(optarg[strlen(optarg)-1] == '\'' || optarg[strlen(optarg)-1] == '\"'){
                    optarg[strlen(optarg)-1] = '\0';
                }
                timestampCol = atoi(optarg);
                break;
            case 'o':
                //Future use for MQTT credentials
                output = optarg;
                outPutCsvFile = optarg;
                outPutCsvFile += "/";
                break;
            default:
                printf("Unknown option, exiting ...\n");
                exit(1);
        }
    }

    if(timestampCol<1){
        printf("Timestamp column must be specified, and it must be above 0. It should follow the following format:\n");
        printf("--timestamps <column (int)>\n");
        exit(1);
    }
}
void configParameters::column_or_text(int* count, char* token){
    int column = *count / 3;
    // Handle arg here
    if (*count % 3 == 0) {
            // argStruct.cols->currentSize++;
            numberOfCols++;
            //printf("SIZE: %d\n", sizeof(**cols) * (*column_count));
            //*cols = realloc(*cols, sizeof(**cols) * numberOfCols);
            columns &ptr = cols.emplace_back();
            ptr.col = atoi(token);
        //printf("Column: %s\n", token);
    }
    if (*count % 3 == 1) {
        //printf("%s\n", token);
        columns &ptr = cols.back();
        ptr.error = atof(token);
        //printf("Error: %s\n", token);
    }
    if (*count % 3 == 2) {
        columns &ptr = cols.back();
        if (*token == 'A') { // Absolute
            ptr.isAbsolute = 1;
        }
        if (*token == 'R') { // Relative
            ptr.isAbsolute = 0;
        }
        //printf("A/R: %s\n", token);
    }
}


