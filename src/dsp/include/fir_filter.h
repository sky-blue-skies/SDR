#ifndef FIR_FILTER_H
#define FIR_FILTER_H

#include <complex>
#include <span>
#include <vector>

class FirFilter {
 public:
  FirFilter(float cutoff_hz, float sample_rate, int num_taps = 65);

  void process(std::span<const std::complex<float>> in,
                 std::vector<std::complex<float>>& out);

 private:
  std::vector<float> _coeffs;               // filter coefficents h[n]
  std::vector<std::complex<float>> _delay_line;  // delay line (last  N-1 samples)
  size_t                           _write_idx = 0;
  static std::vector<float> design_lowpass(float cutoff_norm, int num_taps);
};

#endif  // FIR_FILTER_H