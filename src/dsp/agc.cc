#include "agc.h"

#include <cmath>

Agc::Agc(float target_rms, float attack)
    : _target_rms(target_rms), _attack(attack) {}

void Agc::process(std::vector<std::complex<float>>& iq) {
  for (auto& s : iq) {
    // Apply current gain
    s *= _gain;

    // Measure instantaneous amplitude
    float amplitude = std::abs(s);

    // Adjust gain toward target
    if (amplitude > 1e-6f) {
      float error = _target_rms - amplitude;
      _gain += _attack * error;
    }

    // Clamp gain to sane range
    if (_gain < 0.01f) _gain = 0.01f;
    if (_gain > 1000.f) _gain = 1000.f;
  }
}