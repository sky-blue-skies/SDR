#ifndef FM_DEMOD_H
#define FM_DEMOD_H

#include <complex>
#include <span>
#include <vector>

class FmDemod {
 public:
  FmDemod() = default;

  // Process a block of IQ samples -> audio samples
  void process(std::span<const std::complex<float>> iq, std::vector<float>& out,
               float sample_rate);

 private:
  std::complex<float> _prev = {1.f,
                               0.f};  // previous sample, init to unit phasor
};

#endif  // FM_DEMOD_H