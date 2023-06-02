#include <iostream>
#include <sstream>
#include <cmath>
#include "DecompressManager.h"

void DecompressManager::decompressModels(){
    std::cout << "Starting decompression ..." << std::endl;
    std::map<int, std::deque<std::pair<int, float>>> timeseries;
    std::fstream originalFileStream;
    originalFileStream.open("../" + this->configManager.inputFile, std::ios::in);
    if(!originalFileStream)
    {
        std::cout << "Can't open original file!" << std::endl;
    }
    std::string header;
    std::getline(originalFileStream, header);

    std::fstream csvFileStream;
    csvFileStream.open(std::string("../").append(budgetManager.name), std::ios::in);

    if(!csvFileStream)
    {
        std::cout << "Can't open file!" << std::endl;
    }


    std::vector<Model> models;

    std::vector<std::string> row;
    std::string line, word;
    int lineNO = 0;


    int latestTSOri = getNextLineInOriginalFile(originalFileStream, timeseries);


    while (std::getline(csvFileStream, line)) {

        row.clear();
        std::stringstream s(line);
        Model m;
        int count = 0;
        while (std::getline(s, word, ',')) {
            switch(count){
                case 0:
                    m.start = std::stoi(word);
                    break;
                case 1:
                    m.end = std::stoi(word);
                    break;
                case 2:
                    m.errorBound = std::stof(word);
                    break;
                case 3:
                    m.length = std::stoi(word);
                    break;
                case 4:
                    m.MID = std::stoi(word);
                    break;
                case 5:
                    m.CID = std::stoi(word);
                    break;
                case 6:
                    std::stringstream ss(word);
                    std::string token;
                    while (std::getline(ss, token, '-')) {
                        m.values.push_back(std::stoi(token));
                    }
                    break;
            }

            while(m.end > latestTSOri){
                latestTSOri = getNextLineInOriginalFile(originalFileStream, timeseries);
            }


            count++;
            if(count > 6){
                count = 0;
            }
        }
        lineNO++;
        decompressOneModel(m, timeseries[m.CID]);
    }
    std::cout << "Success!" << std::endl;
    csvFileStream.close();

    std::cout << "Number of data points: " << totalPoints << std::endl;
    std::cout << "Avg error:  " << actualTotalError / totalPoints << std::endl;
}

void DecompressManager::decompressOneModel(Model& m, std::deque<std::pair<int, float>>& originalValues){
    double error = 0;
    Swing swing(error);

    switch(m.MID){
        case 0: {
            //pmc
            float val = bytesToFloat(m.values);
            auto res = PmcMean::gridPmcMean(val, m.length);
            calcActualError(originalValues, res, 0, m.errorBound, m.CID);
            break;
        }
        case 1: {
            //swing
            auto floats = bytesToFloats(m.values);
            bool up = m.values.at(8);

            std::vector<long> timestampsSub;

            for(int i = 0; i < m.length; i++){
                timestampsSub.push_back(std::move(originalValues.at(i).first));
            }
            auto res = swing.gridSwing(floats.at(0), floats.at(1),up,timestampsSub, m.length);
            calcActualError(originalValues, res, 1, m.errorBound, m.CID);
            break;
        }
        case 2: {
            //gorilla
            auto res = Gorilla::gridGorilla(m.values, m.values.size(), m.length);
            calcActualError(originalValues, res, 2, m.errorBound, m.CID);
            break;
        }
    }
}

float DecompressManager::bytesToFloat(std::vector<uint8_t> bytes) {
    union ByteFloatUnion{
        uint8_t bytes[4];
        float floatValue;
    };

    ByteFloatUnion u;
    std::copy(bytes.begin(), bytes.end(), u.bytes);
    return u.floatValue;
}

std::vector<float> DecompressManager::bytesToFloats(std::vector<uint8_t> bytes) {

    union ByteFloatUnion{
        uint8_t bytes[8];
        float floatValue[2];
    };

    ByteFloatUnion u;
    std::copy(bytes.begin(), bytes.end()-1, u.bytes); // -1 because the last element is up/down indicator

    std::vector<float> result;
    result.push_back(u.floatValue[0]);
    result.push_back(u.floatValue[1]);
    return result;
}


float DecompressManager::calcActualError(std::deque<std::pair<int, float>> &original, const std::vector<float> &reconstructed,
                                     int modelType, float errorbound, int col) {
    int size = reconstructed.size();
    float result = 0;
    for(int i = 0; i < size; i++){
        float error;

        // Handle division by zero
        if(fabs(original.at(i).second) == 0){
            error = 0;
        } else {
            error = fabs( (reconstructed.at(i) / original.at(i).second  * 100 ) - 100);

        }
        if(error - 0.0001 > errorbound){
            std::cout << "nonon: " << col << " error: " << error << " err bound: " << errorbound << ": " << (modelType == 0 ? "PMC" : (modelType == 1 ? "SWING" : "GORILLA")) << std::endl;
            std::cout << "      original: " << original.at(i).second << ", reconstructed: " << reconstructed.at(i) << std::endl;
        }


        actualTotalError += error;
        totalPoints++;
    }

    //Remove the items from the original vector
    for(int i = 0; i < reconstructed.size(); i++){
        original.pop_front();
    }

    return result;
}


int DecompressManager::getNextLineInOriginalFile(std::fstream& csvFileStream, std::map<int, std::deque<std::pair<int, float>>>& timeseries) {
    std::vector<std::string> row;
    std::string line, word;
    int timestamp = 0;

    if (!csvFileStream.eof()) {
        row.clear();
        std::getline(csvFileStream, line);
        std::stringstream s(line);
        int count = -1;

        while (std::getline(s, word, ',')) {
        count++;
        if(count==0){
            timestamp = std::stoi(word);
            continue;
        }
        timeseries[count].push_back(std::make_pair(timestamp, std::stof(word)));
        }
    }

    return timestamp;
}