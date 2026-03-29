#include "fm_demod.h"

#include <cmath>

void FmDemod::process(const std::vector<std::complex<float>>& iq,
                      std::vector<float>& out) {
  out.reserve(iq.size());
  for (const auto& sample : iq) {
    // Multiply current sample by the conjugate of the last sample
    // Giving us the phase difference between samples
    auto product = sample * std::conj(_prev);

    // atan2 of the result = instantaneous frequency deviation
    float demod = std::atan2(product.imag(), product.real());

    out.push_back(demod);
    _prev = sample;
  }
}