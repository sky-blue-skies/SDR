#ifndef DECIMATOR_H
#define DECIMATOR_H

#include <complex>
#include <vector>

class Decimator {
 public:
  explicit Decimator(int factor);

  // Decimate a block of complex IQ samplesß
  void process(const std::vector<std::complex<float>>& in,
               std::vector<std::complex<float>>& out);

  void process(const std::vector<float>& in, std::vector<float>& out);

  int factor() const { return _factor; }

 private:
  int _factor;
  int _phase = 0;  // tracks position across block boundaries
};
#endif  // DECIMATOR_H