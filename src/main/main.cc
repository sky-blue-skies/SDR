#include <complex>
#include <iostream>
#include <vector>

#include "decimator.h"
#include "deemphasis.h"
#include "fir_filter.h"
#include "fm_demod.h"
#include "rtl_source.h"

constexpr uint32_t freq_Hz = 93'500'000;
constexpr uint32_t sample_rate_hz = 1'024'000;
// Pass frequencies below 100 kHz, reject everything above
constexpr float ch_bw_hz = 100'000.f;
constexpr int decim_rf = 4;     // 1'024'000 -> 256'000 Hz
constexpr int decim_audio = 6;  // 256'000 -> 42'677 Hz
constexpr float post_rf_rate = static_cast<float>(sample_rate_hz) / decim_rf;
constexpr float tau_us = 50.f;  // UK/EU de-emphasis

enum class Mode { AudioDemod, Power };

Mode parse_args(int argc, char* argv[]) {
  for (int i = 1; i < argc; ++i) {
    if (std::string_view(argv[i]) == "--power") return Mode::Power;
  }
  return Mode::AudioDemod;
}

int main(int argc, char* argv[]) {
  const Mode mode = parse_args(argc, argv);

  RtlSource sdr(freq_Hz, sample_rate_hz);

  // RF chain: filter then decimate down to 256 kHz
  FirFilter lpf(ch_bw_hz, static_cast<float>(sample_rate_hz), 65);
  Decimator rf_decim(decim_rf);

  // Audio chain: demod then decimate down to 43 kHz
  FmDemod demod;
  Deemphasis deemph(tau_us, post_rf_rate);
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
        // Audio stage: demod -> de-emphasis -> decimate to 42 kHz
        std::vector<float> demodulated = demod.process(decimated);  // FM Demod
        std::vector<float> demphasised = deemph.process(demodulated);

        // wrap in complex for decimator
        std::vector<std::complex<float>> audio_cx;
        audio_cx.reserve(demphasised.size());
        for (float s : demphasised) audio_cx.emplace_back(s, 0.f);

        auto audio = audio_decim.process(audio_cx);

        float mean = 0.f;
        for (auto& s : audio) mean += std::abs(s.real());
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