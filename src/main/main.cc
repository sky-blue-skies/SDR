#include <fftw3.h>

#include <algorithm>
#include <atomic>
#include <complex>
#include <csignal>
#include <fstream>
#include <iostream>
#include <numbers>
#include <ranges>
#include <string_view>
#include <thread>
#include <vector>

#include "agc.h"
#include "audio_sink.h"
#include "dc_blocker.h"
#include "decimator.h"
#include "deemphasis.h"
#include "fir_filter.h"
#include "fm_demod.h"
#include "rtl_source.h"

constexpr uint32_t freq_Hz = 88'100'000;
constexpr uint32_t sample_rate_hz = 1'152'000;
constexpr float channel_bw_hz = 100'000.f;
constexpr float max_deviation_hz = 75'000.f * 0.5f;
constexpr int decim_rf = 4;
constexpr int decim_audio = 6;
constexpr float post_rf_rate = static_cast<float>(sample_rate_hz) / decim_rf;
constexpr float audio_rate = post_rf_rate / decim_audio;
constexpr float tau_us = 50.f;

constexpr uint32_t scan_freqs[] = {
    88'100'000, 89'100'000, 91'300'000, 93'500'000,  94'300'000,
    95'800'000, 98'800'000, 99'700'000, 103'500'000, 104'900'000,
};

static std::atomic<bool> running{true};

enum class Mode { AudioDemod, Power, Scan, Fft };

Mode parse_args(int argc, char* argv[]) {
  for (int i = 1; i < argc; ++i) {
    if (std::string_view(argv[i]) == "--power") return Mode::Power;
    if (std::string_view(argv[i]) == "--scan") return Mode::Scan;
    if (std::string_view(argv[i]) == "--fft") return Mode::Fft;
  }
  return Mode::AudioDemod;
}

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
    struct Result {
      uint32_t freq;
      float power_db;
    };
    std::vector<Result> results;
    for (uint32_t freq : scan_freqs) {
      RtlSource sdr(freq, sample_rate_hz);
      float db = measure_power(sdr);
      results.push_back({freq, db});
      std::cout << "  " << freq / 1e6f << " MHz -> " << db << " dBFS\n";
    }
    std::ranges::sort(results, [](const Result& a, const Result& b) {
      return a.power_db > b.power_db;
    });
    std::cout << "\n── Ranked ──────────────────────────\n";
    for (auto& r : results)
      std::cout << "  " << r.freq / 1e6f << " MHz  →  " << r.power_db
                << " dBFS\n";
    std::cout << "\nStrongest: " << results[0].freq / 1e6f << " MHz\n\n";
    return 0;
  }

  RtlSource sdr(freq_Hz, sample_rate_hz);

  if (mode == Mode::Fft) {
    auto raw = sdr.read(16'384);
    const int N = static_cast<int>(raw.size() / 2);

    std::vector<std::complex<float>> iq(N);
    for (size_t i = 0; i < static_cast<size_t>(N); ++i) {
      iq[i] = {(raw[i * 2] - 127.5f) / 127.5f,
               (raw[i * 2 + 1] - 127.5f) / 127.5f};
    }

    fftwf_complex* in = fftwf_alloc_complex(N);
    fftwf_complex* out = fftwf_alloc_complex(N);
    fftwf_plan plan =
        fftwf_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    for (int i = 0; i < N; ++i) {
      float w = 0.5f *
                (1.f - std::cos(2.f * std::numbers::pi_v<float> * i / (N - 1)));
      in[i][0] = iq[i].real() * w;
      in[i][1] = iq[i].imag() * w;
    }
    fftwf_execute(plan);

    const char* home = std::getenv("HOME");
    std::string csv_path = std::string(home) + "/spectrum.csv";
    std::ofstream csv(csv_path);
    csv << "frequency_hz,power_db\n";
    const float bin_width = static_cast<float>(sample_rate_hz) / N;
    for (int k = 0; k < N; ++k) {
      int shifted = (k + N / 2) % N;
      float freq = (shifted - N / 2) * bin_width;
      float re = out[k][0], im = out[k][1];
      float power = 10.f * std::log10((re * re + im * im) / (N * N) + 1e-12f);
      csv << freq << "," << power << "\n";
    }
    std::cout << "Wrote " << csv_path << "\n";
    fftwf_destroy_plan(plan);
    fftwf_free(in);
    fftwf_free(out);
    return 0;
  }

  // ── Audio / Power modes ───────────────────────────────────────────────────

  FirFilter lpf(channel_bw_hz, static_cast<float>(sample_rate_hz), 257);
  Decimator rf_decim(decim_rf);
  FmDemod demod;
  Agc agc(0.5f, 0.01f);
  DcBlocker dc_blocker;
  Deemphasis deemph(tau_us, post_rf_rate);
  Decimator audio_decim(decim_audio);
  AudioSink sink(audio_rate, max_deviation_hz);

  std::signal(SIGINT, [](int) { running = false; });

  if (mode == Mode::Power) {
    while (running) {
      auto raw = sdr.read(16'384);
      float power = 0.f;
      for (size_t i = 0; i + 1 < raw.size(); i += 2) {
        float I = (raw[i] - 127.5f) / 127.5f;
        float Q = (raw[i + 1] - 127.5f) / 127.5f;
        power += I * I + Q * Q;
      }
      power /= static_cast<float>(raw.size() / 2);
      std::cout << "Power: " << 10.f * std::log10(power + 1e-12f) << " dBFS\n";
    }
    return 0;
  }

  // AudioDemod — SDR runs on its own thread, PortAudio on its own thread
  std::thread sdr_thread([&]() {
    std::vector<std::complex<float>> iq;
    std::vector<std::complex<float>> filtered;
    std::vector<std::complex<float>> decimated;
    std::vector<float> demodulated;
    std::vector<float> deemphasised;
    std::vector<float> audio;

    while (running) {
      auto raw = sdr.read(16'384);

      iq.resize(raw.size() / 2);
      for (size_t i = 0; i < iq.size(); ++i) {
        float I = (raw[i * 2] - 127.5f) / 127.5f;
        float Q = (raw[i * 2 + 1] - 127.5f) / 127.5f;
        iq[i] = {I, Q};
      }

      dc_blocker.process(iq);
      lpf.process(iq, filtered);
      rf_decim.process(filtered, decimated);
      agc.process(decimated);
      demod.process(decimated, demodulated, post_rf_rate);
      deemph.process(demodulated, deemphasised);
      audio_decim.process(deemphasised, audio);
      sink.write(audio);
    }
  });

  sdr_thread.join();
  return 0;
}