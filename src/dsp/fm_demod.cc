#include "fm_demod.h"

#include <cmath>
#include <numbers>

void FmDemod::process(const std::vector<std::complex<float>>& iq,
                      std::vector<float>& out, float sample_rate) {
  out.clear();
  out.reserve(iq.size());

  const float scale = sample_rate / (2.f * std::numbers::pi_v<float>);

  for (const auto& sample : iq) {
    auto product = sample * std::conj(_prev);
    float demod = std::atan2(product.imag(), product.real()) * scale;
    out.push_back(demod);
    _prev = sample;
  }
}