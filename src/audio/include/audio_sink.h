#ifndef AUDIO_SINK_H
#define AUDIO_SINK_H

#include <portaudio.h>

#include <atomic>
#include <cstddef>
#include <stdexcept>
#include <vector>

class AudioSink {
 public:
  // sample_rate — audio sample rate in Hz (e.g. 42667)
  // buffer_size — ring buffer size in samples
  explicit AudioSink(float sample_rate, size_t buffer_size = 65536);
  ~AudioSink();

  // Non-copyable — owns hardware resource
  AudioSink(const AudioSink&) = delete;
  AudioSink& operator=(const AudioSink&) = delete;

  // Push audio samples from the SDR thread
  // Blocks if the ring buffer is full
  void write(const std::vector<float>& samples);

 private:
  // PortAudio callback — called from audio thread
  static int pa_callback(const void* input, void* output,
                         unsigned long frame_count,
                         const PaStreamCallbackTimeInfo* time_info,
                         PaStreamCallbackFlags status_flags, void* user_data);

  int callback(float* output, unsigned long frame_count);

  // Simple lock-free ring buffer
  std::vector<float> _ring;
  std::atomic<size_t> _write_pos{0};
  std::atomic<size_t> _read_pos{0};
  size_t _capacity;

  PaStream* _stream = nullptr;
  float _sample_rate;

  // Normalisation scale — FM max deviation is 75kHz
  // at 256kHz post-RF rate, full scale = π radians
  // we scale down to [-1, 1] for the DAC
  static constexpr float k_scale = 1.f / 3.14159265f;
};
#endif  // AUDIO_SINK_H