#include <iostream>
#include <vector>
#include <cmath>
#include <numeric>
#include <limits>

class OutlierDetector {
public:
    explicit OutlierDetector(double thresholdValue, size_t numberOfColumns) {
      for(int i=0; i < numberOfColumns; i++){
        threshold.push_back(thresholdValue);
      }
    }

    bool addValueAndDetectOutlier(int column_number, double value) {
      ++count[column_number];
      double delta = value - mean[column_number];
      mean[column_number] += delta / count[column_number];
      double delta2 = value - mean[column_number];
      m2[column_number] += delta * delta2;

      double std_dev = std::sqrt(m2[column_number] / count[column_number]);
      double z_score = std_dev > std::numeric_limits<double>::epsilon() ? (value - mean[column_number]) / std_dev : 0.0;

      if (std::abs(z_score) > threshold[column_number]) {
          return true;
      }
      else{
        return false;
      }
    }

    std::vector<double> threshold;
    std::vector<int> count;
    std::vector<double> mean;
    std::vector<double> m2;
};