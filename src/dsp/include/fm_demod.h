#ifndef FM_DEMOD_H
#define FM_DEMOD_H

#include <complex>
#include <vector>

class FmDemod {
 public:
  FmDemod() = default;

  // Process a block of IQ samples -> audio samples
  std::vector<float> process(const std::vector<std::complex<float>>& iq);

 private:
  std::complex<float> _prev = {1.f,
                               0.f};  // previous sample, init to unit phasor
};

#endif  // FM_DEMOD_H