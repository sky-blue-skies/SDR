#include <algorithm>
#include <complex>
#include <iostream>
#include <ranges>
#include <string_view>
#include <vector>

#include "audio_sink.h"
#include "decimator.h"
#include "deemphasis.h"
#include "fir_filter.h"
#include "fm_demod.h"
#include "rtl_source.h"

constexpr uint32_t freq_Hz = 93'500'000;
constexpr uint32_t sample_rate_hz = 1'152'000;
constexpr float channel_bw_hz = 100'000.f;
constexpr int decim_rf = 4;
constexpr int decim_audio = 6;
constexpr float post_rf_rate =
    static_cast<float>(sample_rate_hz) / decim_rf;        // 288'000
constexpr float audio_rate = post_rf_rate / decim_audio;  // 48'000 Hz
constexpr float tau_us = 50.f;

// Candidate frequencies to scan (Hz)
constexpr uint32_t scan_freqs[] = {
    88'100'000,   // BBC Radio 2
    89'100'000,   // BBC Radio 2 (alt)
    91'300'000,   // BBC Radio 3
    93'500'000,   // BBC Radio 4
    94'300'000,   // BBC Radio 4 (alt)
    95'800'000,   // BBC Radio 1
    98'800'000,   // BBC Radio 1 (alt)
    99'700'000,   // BBC Radio 1
    103'500'000,  // BBC Radio 4 (alt)
    104'900'000,  // BBC Radio 2 (alt)
};

enum class Mode { AudioDemod, Power, Scan };

Mode parse_args(int argc, char* argv[]) {
  for (int i = 1; i < argc; ++i) {
    if (std::string_view(argv[i]) == "--power") return Mode::Power;
    if (std::string_view(argv[i]) == "--scan") return Mode::Scan;
  }
  return Mode::AudioDemod;
}

// Measure average power in dBFS over n_blocks blocks
float measure_power(RtlSource& sdr, int n_blocks = 4) {
  float power = 0.f;
  int total = 0;

  for (int b = 0; b < n_blocks; ++b) {
    auto raw = sdr.read(16'384);
    for (size_t i = 0; i + 1 < raw.size(); i += 2) {
      float I = (raw[i] - 127.5f) / 127.5f;
      float Q = (raw[i + 1] - 127.5f) / 127.5f;
      power += I * I + Q * Q;
      ++total;
    }
  }

  power /= static_cast<float>(total);
  return 10.f * std::log10(power + 1e-12f);
}

int main(int argc, char* argv[]) {
  const Mode mode = parse_args(argc, argv);

  if (mode == Mode::Scan) {
    std::cout << "\nScanning FM frequencies...\n\n";

    // Collect results
    struct Result {
      uint32_t freq;
      float power_db;
    };
    std::vector<Result> results;

    for (uint32_t freq : scan_freqs) {
      // Re-open device at each frequnecy
      RtlSource sdr(freq, sample_rate_hz);
      float db = measure_power(sdr);
      results.push_back({freq, db});
      std::cout << " " << freq / 1e6f << " MHz -> " << db << " dBFS\n";
    }

    // Sort strongest first
    std::ranges::sort(results, [](const Result& a, const Result& b) {
      return a.power_db > b.power_db;
    });

    for (auto& r : results) {
      std::cout << "  " << r.freq / 1e6f << " MHz  →  " << r.power_db
                << " dBFS\n";
    }
    std::cout << "\nStrongest: " << results[0].freq / 1e6f << " MHz\n\n";
    return 0;
  }

  RtlSource sdr(freq_Hz, sample_rate_hz);
  FirFilter lpf(channel_bw_hz, static_cast<float>(sample_rate_hz), 257);
  Decimator rf_decim(decim_rf);
  FmDemod demod;
  Deemphasis deemph(tau_us, post_rf_rate);
  Decimator audio_decim(decim_audio);
  AudioSink sink(audio_rate);

  // Buffers — allocated once, reused every block
  std::vector<std::complex<float>> iq;
  std::vector<std::complex<float>> filtered;
  std::vector<std::complex<float>> decimated;
  std::vector<float> demodulated;
  std::vector<float> deemphasised;
  std::vector<float> audio;

  while (true) {
    auto raw = sdr.read(16'384);

    // uint8 IQ → complex<float>
    iq.resize(raw.size() / 2);
    for (size_t i = 0; i + 1 < raw.size(); i += 2) {
      float I = (raw[i] - 127.5f) / 127.5f;
      float Q = (raw[i + 1] - 127.5f) / 127.5f;
      iq[i / 2] = {I, Q};
    }

    // RF stage
    lpf.process(iq, filtered);
    rf_decim.process(filtered, decimated);

    std::cout << "raw=" << raw.size() << " iq=" << iq.size()
              << " filtered=" << filtered.size()
              << " decimated=" << decimated.size() << "\n";

    switch (mode) {
      case Mode::AudioDemod: {
        demod.process(decimated, demodulated);
        deemph.process(demodulated, deemphasised);
        audio_decim.process(deemphasised, audio);

        float mean = 0.f;
        for (float s : audio) mean += std::abs(s);
        mean /= static_cast<float>(audio.size());
        std::cout << "mean=" << mean << "\n";

        sink.write(audio);
        break;
      }

      case Mode::Power: {
        float power = 0.f;
        for (auto& s : iq) power += std::norm(s);
        power /= static_cast<float>(iq.size());
        float power_db = 10.f * std::log10(power + 1e-12f);
        std::cout << "Power: " << power_db << " dBFS\n";
        break;
      }

      case Mode::Scan:
        break;  // handled above
    }
  }
}