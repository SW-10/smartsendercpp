#ifndef SWING
#define SWING

//
// Created by power on 05-10-2022.
//

#include <cstddef>
#include <cstdint>
#include <vector>

struct slopeAndIntercept {
    double slope;
    double intercept;
} typedef slopeAndIntercept;


class Swing {
private:
    /// Maximum relative error for the value of each data point.
    double error_bound;
    /// Time at which the first value represented by the current model was
    /// collected.
    long first_timestamp;
    /// Time at which the last value represented by the current model was
    /// collected.
    long last_timestamp;
    /// First value in the segment the current model is fitted to.
    double first_value; // f64 instead of Value to remove casts in fit_value()
    /// Slope for the linear function specifying the upper bound for the current
    /// model.
    double upper_bound_slope;
    /// Intercept for the linear function specifying the upper bound for the
    /// current model.
    double upper_bound_intercept;
    /// Slope for the linear function specifying the lower bound for the current
    /// model.
    double lower_bound_slope;
    /// Intercept for the linear function specifying the lower bound for the
    /// current model.
    double lower_bound_intercept;
    /// The number of data points the current model has been fitted to.
    int length;
    bool error_absolute;

    int isNan(double val);
    int equalOrNAN(double v1, double v2);

    float get_bytes_per_value_swing();
    void get_model_swing(float arr[]);
    size_t get_length_swing();

    void resetSwing();
    
    slopeAndIntercept compute_slope_and_intercept(
        long Timestamp,
        double first_value,
        long final_timestamp,
        double final_value);
    slopeAndIntercept decode_and_compute_slope_and_intercept(long firstTimestamp, long lastTimestamp, double min_value, double max_value, int value);

public:
    Swing(double error_bound, bool is_error_absolute);
    int fitValueSwing(long timestamp, double value);
    std::vector<float> gridSwing(float min, float max, uint8_t values, std::vector<long> timestamps,int timestamp_count);
    // Swing getSwing(double error_bound);
    double getModelFirst();
    double getModelLast();

    double get_error_bound(){ return error_bound; }
    long   get_first_timestamp(){ return first_timestamp; }
    long   get_last_timestamp(){ return last_timestamp; }
    double get_first_value(){ return first_value; }
    double get_upper_bound_slope(){ return upper_bound_slope; }
    double get_upper_bound_intercept(){ return upper_bound_intercept; }
    double get_lower_bound_slope(){ return lower_bound_slope; }
    double get_lower_bound_intercept(){ return lower_bound_intercept; }

} typedef Swing;


#endif