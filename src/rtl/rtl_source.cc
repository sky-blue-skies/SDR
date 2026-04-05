#include "rtl_source.h"

#include <iostream>

RtlSource::RtlSource(uint32_t freq_hz, uint32_t sample_rate)
    : _sample_rate(sample_rate), _freq_hz(freq_hz) {
  if (rtlsdr_open(&_dev, 0) < 0)
    throw std::runtime_error("Failed to open RTL-SDR device");

  rtlsdr_set_sample_rate(_dev, _sample_rate);
  rtlsdr_set_center_freq(_dev, _freq_hz);
  rtlsdr_set_tuner_gain_mode(_dev, 0);
  rtlsdr_reset_buffer(_dev);

  std::cout << "[RTL] Opened device\n"
            << "    Freq:         " << _freq_hz / 1e6 << " MHz\n"
            << "    Sample rate:  " << _sample_rate / 1e3 << " kHz\n"
            << "    Actual rate:  " << rtlsdr_get_sample_rate(_dev) / 1e3
            << " kHz\n";
}

RtlSource::~RtlSource() {
  stop();
  if (_dev) {
    rtlsdr_close(_dev);
    std::cout << "[RTL] Device closed\n";
  }
}

void RtlSource::start(Callback cb) {
  _callback = std::move(cb);
  _running = true;

  _async_thread = std::thread([this]() {
    // Blocks until rtlsdr_cancel_async is called
    rtlsdr_read_async(_dev, &RtlSource::rtlsdr_callback, this,
                      0,       // default num buffers
                      16384);  // bytes per buffer
  });
}

void RtlSource::stop() {
  if (_running.exchange(false)) {
    rtlsdr_cancel_async(_dev);
    if (_async_thread.joinable()) _async_thread.join();
  }
}

void RtlSource::rtlsdr_callback(unsigned char* buf, uint32_t len, void* ctx) {
  auto* self = static_cast<RtlSource*>(ctx);
  if (self->_running && self->_callback) self->_callback(buf, len);
}