#pragma once

#include <rtl-sdr.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <thread>
#include <vector>

class RtlSource {
 public:
  using Callback = std::function<void(const uint8_t*, size_t)>;

  explicit RtlSource(uint32_t freq_hz, uint32_t sample_rate = 1'024'000);
  ~RtlSource();

  RtlSource(const RtlSource&) = delete;
  RtlSource& operator=(const RtlSource&) = delete;

  // Start streaming — callback is called from the async thread
  void start(Callback cb);
  void stop();
  void retune(uint32_t freq_hz);

  uint32_t sample_rate() const { return _sample_rate; }
  uint32_t freq_hz() const { return _freq_hz; }
  uint32_t actual_sample_rate() const { return rtlsdr_get_sample_rate(_dev); }

 private:
  static void rtlsdr_callback(unsigned char* buf, uint32_t len, void* ctx);

  rtlsdr_dev_t* _dev = nullptr;
  uint32_t _sample_rate = 0;
  uint32_t _freq_hz = 0;
  Callback _callback;
  std::thread _async_thread;
  std::atomic<bool> _running{false};
};