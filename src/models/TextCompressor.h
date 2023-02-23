#pragma once

#include <cstdlib>
#include <string>
#include <vector>

class TextCompressor{
 public:
  std::string string;
  std::vector<long> timestamps;
  int id;

  TextCompressor(int id);
  bool fitString(std::string text, long timestamp);

};