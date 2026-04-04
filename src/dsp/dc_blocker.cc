#include "dc_blocker.h"

void DcBlocker::process(std::vector<std::complex<float>>& iq) {
  for (auto& s : iq) {
    // Track the running mean of I and Q separately
    _avg_i = alpha * _avg_i + (1.f - alpha) * s.real();
    _avg_q = alpha * _avg_q + (1.f - alpha) * s.imag();

    // Subtract the DC estimate
    s = {s.real() - _avg_i, s.imag() - _avg_q};
  }
}