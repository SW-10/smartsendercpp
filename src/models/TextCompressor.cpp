#include "../doctest.h"
#include "TextCompressor.h"

TextCompressor::TextCompressor(int id){
  this->id = id;
}

bool TextCompressor::fitString(std::string text, long timestamp){
  if(this->timestamps.empty()){
    this->string = text;
    this->timestamps.push_back(timestamp);
    return true;
  }else if((this->string.compare(text) == 0)){ //compare() returns 0 if they are equal
    this->timestamps.push_back(timestamp);
    return true;
  }
  return false;
}

TEST_CASE("Same values return 1"){

  TextCompressor TextCompressor(1);

  std::vector<int> checks;
  std::vector<int> timestamps = {0, 10,20,30,39,48,55,62,69,76};
  for(int i = 0; i < timestamps.size(); i++){
    checks.push_back(TextCompressor.fitString("STATUS:OK",timestamps[i]));
  };
  std::vector<int> comparison = {1,1,1,1,1,1,1,1,1,1};

  CHECK_EQ(checks, comparison);

}

TEST_CASE("Different values return 0"){
  TextCompressor TextCompressor(1);

  std::vector<int> checks;
  std::vector<int> timestamps = {0, 10,20,30,39,48,55,62,69,76};
  for(int i = 0; i < timestamps.size(); i++){
    checks.push_back(TextCompressor.fitString(("STATUS:" + std::to_string(i)),timestamps[i]));
  };
  std::vector<int> comparison = {1,0,0,0,0,0,0,0,0,0};

  CHECK_EQ(checks, comparison);

}