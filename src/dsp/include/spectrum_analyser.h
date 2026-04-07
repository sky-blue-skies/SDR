#ifndef SPECTRUM_ANALYSER_H
#define SPECTRUM_ANALYSER_H

#include <complex>
#include <functional>
#include <mutex>
#include <vector>

//  Computes a rolling averaged power spectrum from a stream of IQ samples.
//  Thread-safe - process( ccan be called from the SDR callback thread while
//  latest_db() is called from a UI thread.
class SpectrumAnalyser {
 public:
  // fft_size     - number of FFT bins (power of 2, e.g. 2048)
  // sample_rate  - IQ sample rate in Hz (used to label frequencies)
  // avg_count    - number of FFTs to average before publishing (smoothing)
  explicit SpectrumAnalyser(int fft_size, float sample_rate, int avg_count = 4);

  ~SpectrumAnalyser();

  // Non-copyable
  SpectrumAnalyser(const SpectrumAnalyser&) = delete;
  SpectrumAnalyser& operator=(const SpectrumAnalyser&) = delete;

  // Feed IQ samples - call from SDR callback thread
  void process(const std::vector<std::complex<float>>& iq);

  // Get latest averaged spectrum as dBFS values - call from UI thread
  // Returns false if no new data is available sinbce lsy call
  bool latest_db(std::vector<float>& out_db);

  int fft_size() const { return _fft_size; }
  float sample_rate() const { return _sample_rate; }
  float bin_width() const { return _sample_rate / _fft_size; }

 private:
  void compute_fft();

  int _fft_size;
  float _sample_rate;
  int _avg_count;

  // FFTW
  void* _fftw_in;    // fftwf_complex*
  void* _fftw_out;   // fftwf_complex*
  void* _fftw_plan;  // fftwf_plan

  // Hann window coefficents
  std::vector<float> _window;

  // Accumulation buffer -- filled sample by sample from process()
  std::vector<std::complex<float>> _accum;
  int _accum_pos = 0;

  // Averaging accumulator
  std::vector<float> _avg_power;
  int _avg_pos = 0;

  // Published result - written by SDR thread, read by UI thread
  std::mutex _result_mutex;
  std::vector<float> _result_db;
  bool _result_ready = false;
};

#endif  // SPECTRUM_ANALYSER_H