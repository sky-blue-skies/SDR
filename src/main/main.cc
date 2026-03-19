#include <complex>
#include <iostream>
#include <vector>

#include "rtl_source.h"

constexpr uint32_t freq_Hz = 93'500'000;
constexpr uint32_t bandwidth_Hz = 1'024'000;
int main() {
  RtlSource sdr(freq_Hz, bandwidth_Hz);
  for (int block = 0; block < 10; ++block) {
    auto raw = sdr.read(16384);

    // Convert interleaved uint8_t IQ -> complex<float> in [-1,1]
    std::vector<std::complex<float>> iq;
    iq.reserve(raw.size() / 2);
    for (size_t i = 0; i + 1 < raw.size(); i += 2) {
      float I = (raw[i] - 127.5f) / 127.5f;
      float Q = (raw[i + 1] - 127.5f) / 127.5f;
      iq.emplace_back(I, Q);
    }

    // Average power in dBFS
    float power = 0.f;
    for (auto& s : iq) power += std::norm(s);  // norm = I^2 + Q^2
    power /= static_cast<float>(iq.size());

    float power_db = 10.f * std::log10(power + 1e-12f);
    std::cout << "Block " << block << " - Power: " << power_db << " dBFS\n";
  }
}