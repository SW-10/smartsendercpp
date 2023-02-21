//
// Created by fragn on 21/02/2023.
//

#ifndef SMARTSENDERCPP_SRC_MODELS_TEXTRLE_H_
#define SMARTSENDERCPP_SRC_MODELS_TEXTRLE_H_

#endif //SMARTSENDERCPP_SRC_MODELS_TEXTRLE_H_

#include <cstdlib>
#include <string>
#include <vector>

class TextCompressor{
 public:
  std::string string;
  std::vector<long> timestamps;
  int id;

  TextCompressor(int id);
  int fitString(std::string string, long timestamp);


};