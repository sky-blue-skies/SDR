#ifndef DEEMPHASIS_H
#define DEEMPHASIS_H

#include <vector>

class Deemphasis {
 public:
  // tau_us - time constant in microseconds (50 for eu/uk, 75 for US)
  // sample_rate - sample rate of the input signal
  explicit Deemphasis(float tau_us, float sample_rate);

  std::vector<float> process(const std::vector<float>& in);

 private:
  float _alpha;
  float _prev;
};

#endif  // DEEMPHASIS_H