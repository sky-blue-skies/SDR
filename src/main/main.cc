#include <complex>
#include <iostream>
#include <vector>

#include "fm_demod.h"
#include "rtl_source.h"

constexpr uint32_t freq_Hz = 93'500'000;
constexpr uint32_t bandwidth_Hz = 1'024'000;

enum class Mode { AudioDemod, Power };

Mode parse_args(int argc, char* argv[]) {
  for (int i = 1; i < argc; ++i) {
    if (std::string_view(argv[i]) == "--power") return Mode::Power;
  }
  return Mode::AudioDemod;
}

int main(int argc, char* argv[]) {
  const Mode mode = parse_args(argc, argv);

  RtlSource sdr(freq_Hz, bandwidth_Hz);
  FmDemod demod;

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

    switch (mode) {
      case Mode::AudioDemod: {
        std::vector<float> audio = demod.process(iq);
        float mean = 0.f;
        for (float s : audio) mean += std::abs(s);
        mean /= static_cast<float>(audio.size());

        std::cout << "Block " << block << " - Audio mean amplitude: " << mean
                  << "\n";
        break;
      }
      case Mode::Power: {
        // Average power in dBFS
        float power = 0.f;
        for (auto& s : iq) power += std::norm(s);  // norm = I^2 + Q^2
        power /= static_cast<float>(iq.size());

        float power_db = 10.f * std::log10(power + 1e-12f);
        std::cout << "Block " << block << " - Power: " << power_db << " dBFS\n";
      }
    }
  }
}