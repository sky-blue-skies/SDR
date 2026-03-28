#ifndef DECIMATOR_H
#define DECIMATOR_H

#include <vector>
#include <complex>

class Decimator {
 public:
  explicit Decimator(int factor);

  // Decimate a block of complex IQ samplesß
  std::vector<std::complex<float>> process(
      const std::vector<std::complex<float>>& in);

  int factor() const { return _factor; }

 private:
  int _factor;
  int _phase = 0;  // tracks position across block boundaries
};
#endif  // DECIMATOR_H