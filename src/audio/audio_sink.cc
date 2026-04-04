#include "audio_sink.h"

#include <cstring>
#include <iostream>

// ── Construction
// ──────────────────────────────────────────────────────────────

AudioSink::AudioSink(float sample_rate, float max_deviation_hz,
                     size_t buffer_size)
    : _ring(buffer_size, 0.f),
      _capacity(buffer_size),
      _sample_rate(sample_rate),
      _k_scale(1.f / max_deviation_hz) {
  PaError err = Pa_Initialize();

  if (err != paNoError) throw std::runtime_error(Pa_GetErrorText(err));

  err = Pa_OpenDefaultStream(&_stream,
                             0,          // no input channels
                             1,          // mono output
                             paFloat32,  // 32-bit float samples
                             _sample_rate,
                             256,  // frames per callback buffer
                             &AudioSink::pa_callback,
                             this  // pass 'this' as user data to callback
  );

  if (err != paNoError) {
    Pa_Terminate();
    throw std::runtime_error(Pa_GetErrorText(err));
  }

  err = Pa_StartStream(_stream);
  if (err != paNoError) {
    Pa_CloseStream(_stream);
    Pa_Terminate();
    throw std::runtime_error(Pa_GetErrorText(err));
  }

  std::cout << "[Audio] Opened output stream @ " << _sample_rate << " Hz\n";
}

// ── Destruction
// ───────────────────────────────────────────────────────────────

AudioSink::~AudioSink() {
  if (_stream) {
    Pa_StopStream(_stream);
    Pa_CloseStream(_stream);
  }
  Pa_Terminate();
  std::cout << "[Audio] Stream closed\n";
}

// ── Write (SDR thread)
// ────────────────────────────────────────────────────────

void AudioSink::write(const std::vector<float>& samples) {
  for (float s : samples) {
    // Scale from atan2 range [-π, π] to [-1, 1]
    float scaled = s * _k_scale;

    // Soft clip to [-1, 1] just in case
    if (scaled > 1.f) scaled = 1.f;
    if (scaled < -1.f) scaled = -1.f;

    size_t next = (_write_pos.load(std::memory_order_relaxed) + 1) % _capacity;

    // If buffer is full, wait for the audio thread to consume
    while (next == _read_pos.load(
                       std::memory_order_acquire));  // spin — in practice this
                                                     // should rarely happen

    _ring[_write_pos.load(std::memory_order_relaxed)] = scaled;
    _write_pos.store(next, std::memory_order_release);
  }
}

// ── PortAudio callback (audio thread) ────────────────────────────────────────

int AudioSink::pa_callback(const void* input, void* output,
                           unsigned long frame_count,
                           const PaStreamCallbackTimeInfo* time_info,
                           PaStreamCallbackFlags status_flags,
                           void* user_data) {
  return static_cast<AudioSink*>(user_data)->callback(
      static_cast<float*>(output), frame_count);
}

int AudioSink::callback(float* output, unsigned long frame_count) {
  for (unsigned long i = 0; i < frame_count; ++i) {
    size_t rpos = _read_pos.load(std::memory_order_relaxed);

    if (rpos == _write_pos.load(std::memory_order_acquire)) {
      output[i] = 0.f;
      _underrun_count.fetch_add(1, std::memory_order_relaxed);  // ← add this
    } else {
      output[i] = _ring[rpos];
      _read_pos.store((rpos + 1) % _capacity, std::memory_order_release);
    }
  }
  return paContinue;
}