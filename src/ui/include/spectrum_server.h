#ifndef SPECTRUM_SERVER_H
#define SPECTRUM_SERVER_H

#include <libwebsockets.h>

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// Callback type for tune requests from the browser
using TuneCallback = std::function<void(uint32_t freq_hz)>;

// Foward declare to avoid putting libwebsockets into every header
struct lws_context;

// Broadcasts spectrum data to all connected WebSocket clients.
// Runs its own service thread - thread_safee to call broadcast() from anyother
// thread.

class SpectrumServer {
 public:
  // port        - TCP port to listen on
  // fft_size    - number of FFT bins (must match Spectrum Analayser)
  // sample_rate - used to compute frequency axis in the browser
  // center_freq - tuned frequency in Hz, sent to browser for axis labelling
  explicit SpectrumServer(int port, int fft_size, float sample_rate,
                          uint32_t center_freq);
  ~SpectrumServer();

  // Non-copyable
  SpectrumServer(const SpectrumServer&) = delete;
  SpectrumServer& operator=(const SpectrumServer&) = delete;

  // Push a new spectrum frame to all connected clients
  // Called from SDR thread - must be fast and non-blocking
  void broadcast(const std::vector<float>& db);

  // Update the center frequency label (e.g. after retuning)
  void set_center_freq(uint32_t freq_Hz) {
    _center_freq.store(freq_Hz, std::memory_order_relaxed);
  }

  // Register a callback to be called when browser requests a retune
  void on_tune_request(TuneCallback cb) {
    std::lock_guard lock(_tune_mutex);
    _tune_callback = std::move(cb);
  }

  int port() const { return _port; }
  bool is_running() const { return _running.load(); }

 private:
  // libwebsockets callback - static because lws requires C-style callback
  static int lws_callback(struct lws* wsi, enum lws_callback_reasons reason,
                          void* user, void* in, size_t len);
  void service_thread();

  // Pending frame -  written by broadcast(), read by lws callback
  std::mutex _frame_mutex;
  std::vector<float> _pending_db;
  bool _frame_ready = false;

  int _port;
  int _fft_size;
  float _sample_rate;
  std::atomic<uint32_t> _center_freq;

  struct lws_context* _context = nullptr;
  std::thread _thread;
  std::atomic<bool> _running{false};

  std::mutex _tune_mutex;
  TuneCallback _tune_callback;
};
#endif  // SPECTRUM_SERVER_H