#include "fir_filter.h"

#include <cmath>
#include <numbers>
#include <stdexcept>

// ── Construction ─────────────────────────────────────────────────────────────
FirFilter::FirFilter(float cutoff_hz, float sample_rate, int num_taps) {
  if (num_taps % 2 == 0)
    throw std::invalid_argument("Number of taps must be odd");

  // Normalise cutoff to [0, 1] where 1.0 = sample_rate/2 (Nyquist)
  const float cutoff_norm = cutoff_hz / (sample_rate / 2.f);

  _coeffs = design_lowpass(cutoff_norm, num_taps);
  _delay_line.assign(num_taps, {0.f, 0.f});
  _write_idx = 0;
}

// ── Coefficient design (windowed sinc) ───────────────────────────────────────
std::vector<float> FirFilter::design_lowpass(float cutoff_norm, int num_taps) {
  const int M = num_taps - 1;  // filter order
  const float fc = cutoff_norm;
  const float pi = std::numbers::pi_v<float>;

  std::vector<float> h(num_taps);

  for (int n = 0; n < num_taps; ++n) {
    const float mid = static_cast<float>(M) / 2.f;

    // Windowed sinc
    float sinc;
    if (n == static_cast<int>(mid)) {
      sinc = 2.f * fc;
    } else {
      const float x = n - mid;
      sinc = std::sin(2.f * pi * fc * x) / (pi * x);
    }

    // Hamming window - reduces sidelobes
    const float window = 0.54f - 0.46f * std::cos(2.f * pi * n / M);

    h[n] = sinc * window;
  }

  // Normalise so DC gain = 1.0
  float sum = 0.f;
  for (float c : h) sum += c;
  for (float& c : h) c /= sum;

  return h;
}

// ── Per block processing ───────────────────────────────────────
void FirFilter::process(std::span<const std::complex<float>> in,
                        std::vector<std::complex<float>>& out) {
  out.clear();            // ← clear before filling
  out.resize(in.size());  // ← MUST resize to match input size

  const int taps = static_cast<int>(_coeffs.size());

  for (size_t i = 0; i < in.size(); ++i) {
    // Write a new sample into circular delay_line
    _delay_line[_write_idx] = in[i];  // ← was in[_write_idx], should be in[i]

    // MAC - read backwards from write pointer
    std::complex<float> sum{0.f, 0.f};
    for (size_t k = 0; k < taps; ++k) {
      size_t read_idx = (_write_idx + taps - k) % taps;
      sum += _delay_line[read_idx] * _coeffs[k];
    }

    out[i] = sum;
    _write_idx = (_write_idx + 1) % taps;
  }
}
