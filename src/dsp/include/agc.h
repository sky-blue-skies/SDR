#ifndef AGC_H
#define AGC_H

#include <complex>
#include <vector>

class Agc {
 public:
  // target_rms — what RMS level we want to maintain (e.g. 0.5)
  // attack     — how fast gain increases (e.g. 0.01)
  explicit Agc(float target_rms = 0.5f, float attack = 0.01f);

  void process(std::vector<std::complex<float>>& iq);

 private:
  float _gain = 1.f;
  float _target_rms;
  float _attack;
};

#endif  // AGC_H