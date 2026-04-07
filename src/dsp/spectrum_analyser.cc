#include "spectrum_analyser.h"

#include <fftw3.h>

#include <cmath>
#include <numbers>

// ── Construction
// ----─────────────────────────────────────────────────────────────
SpectrumAnalyser::SpectrumAnalyser(int fft_size, float sample_rate,
                                   int avg_count)
    : _fft_size(fft_size),
      _sample_rate(sample_rate),
      _avg_count(avg_count),
      _accum(fft_size),
      _avg_power(fft_size, 0.f),
      _result_db(fft_size, -120.f) {
  // Allocate FFTW buffers
  _fftw_in = fftwf_alloc_complex(_fft_size);
  _fftw_out = fftwf_alloc_complex(_fft_size);
  _fftw_plan =
      fftwf_plan_dft_1d(_fft_size, static_cast<fftwf_complex*>(_fftw_in),
                        static_cast<fftwf_complex*>(_fftw_out), FFTW_FORWARD,
                        FFTW_MEASURE);  // Measure finds optimal plan at startup
  // Hann Window
  _window.resize(_fft_size);
  const float pi = std::numbers::pi_v<float>;
  for (int n = 0; n < _fft_size; ++n)
    _window[n] = 0.5f * (1.f - std::cos(2.f * pi * n / (_fft_size - 1)));
}

SpectrumAnalyser::~SpectrumAnalyser() {
  fftwf_destroy_plan(static_cast<fftwf_plan>(_fftw_plan));
  fftwf_free(_fftw_in);
  fftwf_free(_fftw_out);
}

void SpectrumAnalyser::process(const std::vector<std::complex<float>>& iq) {
  for (const auto& sample : iq) {
    _accum[_accum_pos] = sample;
    ++_accum_pos;
    if (_accum_pos == _fft_size) {
      compute_fft();
      _accum_pos = 0;
    }
  }
}

void SpectrumAnalyser::compute_fft() {
  auto* in = static_cast<fftwf_complex*>(_fftw_in);

  // Apply Hann window
  for (int n = 0; n < _fft_size; ++n) {
    in[n][0] = _accum[n].real() * _window[n];
    in[n][1] = _accum[n].imag() * _window[n];
  }

  fftwf_execute(static_cast<fftwf_plan>(_fftw_plan));

  auto* out = static_cast<fftwf_complex*>(_fftw_out);

  // Accumulate power into averaging bufffer
  for (int k = 0; k < _fft_size; ++k) {
    float re = out[k][0];
    float im = out[k][1];
    float power = (re * re + im * im) / (_fft_size * _fft_size);
    _avg_power[k] += power;
  }

  ++_avg_pos;

  if (_avg_pos == _avg_count) {
    // Convert averaged power to dBFS with FFT shift
    // (so DC is in the centre, negative freqs on the left)
    std::vector<float> db(_fft_size);
    const int half = _fft_size / 2;

    for (int k = 0; k < _fft_size; ++k) {
      int shifted = (k + half) % _fft_size;
      db[k] = 10.f * std::log10(_avg_power[shifted] / _avg_count + 1e-12f);
    }

    // Publish
    {
      std::lock_guard lock(_result_mutex);
      _result_db = std::move(db);
      _result_ready = true;
    }

    // Reset averaging
    std::fill(_avg_power.begin(), _avg_power.end(), 0.f);
    _avg_pos = 0;
  }
}

bool SpectrumAnalyser::latest_db(std::vector<float>& out_db) {
  std::lock_guard lock(_result_mutex);
  if (!_result_ready) return false;
  out_db = _result_db;
  _result_ready = false;
  return true;
}