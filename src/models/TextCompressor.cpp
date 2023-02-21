//
// Created by fragn on 21/02/2023.
//

#include "../doctest.h"
#include "TextCompressor.h"

TextCompressor::TextCompressor(int id){
  this->id = id;
}

int TextCompressor::fitString(std::string string, long timestamp){
  if(this->timestamps.empty()){
    this->string = string;
    this->timestamps.push_back(timestamp);
    return 1;
  }else if((this->string.compare(string) == 0)){ //compare() returns 0 if they are equal
    this->timestamps.push_back(timestamp);
    return 1;
  }
  return 0;
}

TEST_CASE("Same values return 1"){

  TextCompressor TextCompressor(1);

  std::vector<int> checks;
  std::vector<int> timestamps = {0, 10,20,30,39,48,55,62,69,76};
  for(int i = 0; i < timestamps.size(); i++){
    checks.push_back(TextCompressor.fitString("STATUS:OK",timestamps[i]));
  };

  CHECK_EQ(checks, std::vector<int>(10, 1));

}

TEST_CASE("Different values return 0"){
  TextCompressor TextCompressor(1);

  std::vector<int> checks;
  std::vector<int> timestamps = {0, 10,20,30,39,48,55,62,69,76};
  for(int i = 0; i < timestamps.size(); i++){
    checks.push_back(TextCompressor.fitString(("STATUS:" + std::to_string(i)),timestamps[i]));
  };

  CHECK_EQ(checks, std::vector<int>(10, 0));

}