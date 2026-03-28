#include <complex>
#include <iostream>
#include <vector>

#include "decimator.h"
#include "fir_filter.h"
#include "fm_demod.h"
#include "rtl_source.h"

constexpr uint32_t freq_Hz = 93'500'000;
constexpr uint32_t bandwidth_Hz = 1'024'000;
// Pass frequencies below 100 kHz, reject everything above
constexpr float ch_bw_hz = 100'000.f;
constexpr int decim_rf = 4;     // 1'024'000 -> 256'000 Hz
constexpr int decim_audio = 6;  // 256'000 -> 42'677 Hz

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

  // RF chain: filter then decimate down to 256 kHz
  FirFilter lpf(ch_bw_hz, static_cast<float>(bandwidth_Hz), 65);
  Decimator rf_decim(decim_rf);

  // Audio chain: demod then decimate down to 43 kHz
  FmDemod demod;
  Decimator audio_decim(decim_audio);

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

    // RF stage
    auto filtered = lpf.process(iq);              // 1'024'000 Hz → filtered
    auto decimated = rf_decim.process(filtered);  // → 256'000 Hz

    switch (mode) {
      case Mode::AudioDemod: {
        // Audio stage
        std::vector<float> audio_full = demod.process(decimated);  // FM Demod

        // audio_full is real float - wrap in complex for decimator
        std::vector<std::complex<float>> audio_cx;
        audio_cx.reserve(audio_full.size());
        for (float s : audio_full) audio_cx.emplace_back(s, 0.f);

        auto audio_decimated = audio_decim.process(audio_cx);

        float mean = 0.f;
        for (auto& s : audio_decimated) mean += std::abs(s.real());
        mean /= static_cast<float>(audio_decimated.size());

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