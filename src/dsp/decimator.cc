#include "decimator.h"

#include <stdexcept>

// ── Construction
// ----─────────────────────────────────────────────────────────────
Decimator::Decimator(int factor) : _factor{factor} {
  if (factor < 2) throw std::invalid_argument("Decimation factor must be >= 2");
}

// ── Decimate process
// ─────────────────────────────────────────────────────────────
void Decimator::process(const std::vector<std::complex<float>>& in,
                        std::vector<std::complex<float>>& out) {
  out.clear();
  out.reserve(in.size() / _factor + 1);

  for (size_t i = 0; i < in.size(); ++i) {
    if (_phase == 0) out.push_back(in[i]);
    _phase = (_phase + 1) % _factor;
  }
}

// ─────────────────────────────────────────────────────────────
void Decimator::process(const std::vector<float>& in, std::vector<float>& out) {
  out.clear();
  out.reserve(in.size() / _factor + 1);

  for (size_t i = 0; i < in.size(); ++i) {
    if (_phase == 0) out.push_back(in[i]);
    _phase = (_phase + 1) % _factor;
  }
}