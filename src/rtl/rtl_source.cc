#include "rtl_source.h"

#include <iostream>

RtlSource::RtlSource(uint32_t freq_hz, uint32_t sample_rate)
    : _sample_rate(sample_rate), _freq_hz(freq_hz) {
  if (rtlsdr_open(&_dev, 0) < 0)
    throw std::runtime_error("Failed to open RTL-SDR device");

  rtlsdr_set_sample_rate(_dev, _sample_rate);
  rtlsdr_set_center_freq(_dev, _freq_hz);
  rtlsdr_set_tuner_gain_mode(_dev, 0);  // 0 = auto gain
  // rtlsdr_set_tuner_gain_mode(_dev, 1);  // manual
  // rtlsdr_set_tuner_gain(_dev, 496);     // 49.6 dB

  rtlsdr_reset_buffer(_dev);

  std::cout << "[RTL] Opened device\n"
            << "    Freq:         " << _freq_hz / 1e6 << " MHz\n"
            << "    Sample rate:  " << _sample_rate / 1e3 << " kHz\n";
}

RtlSource::~RtlSource() {
  if (_dev) {
    rtlsdr_close(_dev);
    ;
    std::cout << "[RTL] Device closed\n";
  }
}

std::vector<uint8_t> RtlSource::read(int num_samples) {
  std::vector<uint8_t> buf(num_samples * 2);  // interleaved I, Q bytes
  int n_read = 0;
  rtlsdr_read_sync(_dev, buf.data(), static_cast<int>(buf.size()), &n_read);
  buf.resize(n_read);
  return buf;
}