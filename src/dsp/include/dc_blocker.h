#ifndef DCBLOCKER_H
#define DCBLOCKER_H

#include <complex>
#include <vector>

class DcBlocker {
 public:
  DcBlocker() = default;

  void process(std::vector<std::complex<float>>& iq);

 private:
  float _avg_i = 0.f;
  float _avg_q = 0.f;
  static constexpr float alpha = 0.999f;
};

#endif  // DCBLOCKER_H