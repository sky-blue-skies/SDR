#include "fm_demod.h"

#include <cmath>

void FmDemod::process(const std::vector<std::complex<float>>& iq,
                      std::vector<float>& out) {
  out.clear();  // ← add this
  out.reserve(iq.size());

  for (const auto& sample : iq) {
    auto product = sample * std::conj(_prev);
    float demod = std::atan2(product.imag(), product.real());
    out.push_back(demod);
    _prev = sample;
  }
}