#ifndef RTL_SOURCE_H
#define RTL_SOURCE_H

#include <rtl-sdr.h>

#include <cstdint>
#include <stdexcept>
#include <vector>

class RtlSource {
 public:
  explicit RtlSource(uint32_t freq_hz, uint32_t sample_rate = 1'024'000);
  ~RtlSource();

  // Non-copyable = owns a hardware resource
  RtlSource(const RtlSource&) = delete;
  RtlSource& operator=(const RtlSource&) = delete;
  // Movable
  RtlSource(RtlSource&&) = default;
  RtlSource& operator=(RtlSource&&) = default;

  // Read num_samples IQ pairs as raw uint8
  std::vector<uint8_t> read(int num_samples);

  uint32_t sample_rate() const { return _sample_rate; }
  uint32_t freq_hz() const { return _freq_hz; }

 private:
  rtlsdr_dev_t* _dev = nullptr;
  uint32_t _sample_rate = 0;
  uint32_t _freq_hz = 0;
};

#endif  // RTL_SOURCE_H