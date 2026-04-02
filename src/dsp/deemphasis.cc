#include "deemphasis.h"

#include <stdexcept>

// ── Construction
// ----─────────────────────────────────────────────────────────────
Deemphasis::Deemphasis(float tau_us, float sample_rate) {
  if (sample_rate <= 0.f)
    throw std::invalid_argument("sample_rate must be positive and nonzero");
  if (tau_us <= 0.f)
    throw std::invalid_argument("tau_us must be positive and nonzero");

  const float tau_s = tau_us * 1e-6f;  // µs → seconds
  const float dt = 1.f / sample_rate;  // time per sample

  _alpha = dt / (tau_s + dt);

  // Sanity check — alpha should be small (heavy smoothing)
  // For UK 50µs @ 256kHz: α ≈ 0.0724
}

void Deemphasis::process(const std::vector<float>& in,
                         std::vector<float>& out) {
  out.clear();
  out.resize(in.size());  // ← resize not reserve

  for (size_t n = 0; n < in.size(); ++n) {
    _prev = _alpha * in[n] + (1.f - _alpha) * _prev;
    out[n] = _prev;
  }
}