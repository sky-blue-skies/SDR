#ifndef FIR_FILTER_H
#define FIR_FILTER_H

#include <complex>
#include <vector>

class FirFilter {
 public:
  // cutoff_hz   frequency above which we attenuate
  // sample_rate sample rate of the input signal
  // num_taps    filter length must be odd, more = sharper (try 64+1 = 65)

  FirFilter(float cutoff_hz, float sample_rate, int num_taps = 65);

  // Filter a block of complx IQ samples
  void process(const std::vector<std::complex<float>>& iq,
               std::vector<std::complex<float>>& out);

 private:
  std::vector<float> _coeffs;               // filter coefficents h[n]
  std::vector<std::complex<float>> _delay;  // delay line (last  N-1 samples)
  static std::vector<float> design_lowpass(float cutoff_norm, int num_taps);
};

#endif  // FIR_FILTER_H