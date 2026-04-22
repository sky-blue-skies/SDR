#include "spectrum_server.h"

#include <libwebsockets.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <fstream>

// ── Per-session state
// ───────────────────────────────────────────────────────── libwebsockets
// allocates this struct for each connected client
struct SessionData {
  bool pending = false;  // true if we need to send a frame to the client
};

SpectrumServer::SpectrumServer(int port, int fft_size, float sample_rate,
                               uint32_t center_freq)
    : _port(port),
      _fft_size(fft_size),
      _sample_rate(sample_rate),
      _center_freq(center_freq),
      _pending_db(fft_size, -120.f) {
  // Define the WebSocket protocol
  // libwebsockets require a null-terminated array of protocol descriptors
  static struct lws_protocols protocols[] = {
      {"spectrum",  // protocol name (matched by browser)
       &SpectrumServer::lws_callback,
       sizeof(SessionData),  //  per-session data size
       4096,                 // rx buffer size
       0, nullptr, 0},
      {nullptr, nullptr, 0, 0, 0, nullptr, 0}  // terminator
  };

  struct lws_context_creation_info info = {};
  info.port = _port;
  info.protocols = protocols;
  info.user = this;  // passed to callback as lws_context_user()

  _context = lws_create_context(&info);
  if (!_context)
    throw std::runtime_error("Failed to create libwebsockets context");

  _running = true;
  _thread = std::thread(&SpectrumServer::service_thread, this);

  std::cout << "[WS] Spectrum server listening on ws://localhost:" << _port
            << "\n"
            << "[WS] Open http://localhost:" << _port << " in your browser\n";
}

// ── Destruction
// ───────────────────────────────────────────────────────────────
SpectrumServer::~SpectrumServer() {
  _running = false;
  if (_thread.joinable()) _thread.join();
  if (_context) lws_context_destroy(_context);
}

// ── broadcast() — called from SDR thread ─────────────────────────────────────
void SpectrumServer::broadcast(const std::vector<float>& db) {
  {
    std::lock_guard lock(_frame_mutex);
    _pending_db = db;
    _frame_ready = true;
  }

  // Wake up lws service loop so it sends immediately
  if (_context) lws_cancel_service(_context);
}

// ── Service thread
// ────────────────────────────────────────────────────────────
void SpectrumServer::service_thread() {
  while (_running) {
    // Drive the libwebsockets event loop - returns after -50ms or on cancel
    lws_service(_context, 50);
  }
}

// ── libwebsockets callback
// ────────────────────────────────────────────────────
int SpectrumServer::lws_callback(struct lws* wsi,
                                 enum lws_callback_reasons reason, void* user,
                                 void* in, size_t len) {
  // Retrieve our SpectrumServer instance from the context user pointer
  auto* self =
      static_cast<SpectrumServer*>(lws_context_user(lws_get_context(wsi)));

  auto* session = static_cast<SessionData*>(user);

  switch (reason) {
    case LWS_CALLBACK_ESTABLISHED:
      std::cout << "[WS] Client connected\n";
      session->pending = false;
      break;

    case LWS_CALLBACK_CLOSED:
      std::cout << "[WS] Client disconnected\n";
      break;

    case LWS_CALLBACK_SERVER_WRITEABLE: {
      // Build JSON frame
      // Format: {"sr":960000, "cf":88100000, "bins":[-80.1, -79.2,....]}
      std::ostringstream json;
      json << "{\"sr\":" << self->_sample_rate
           << ",\"cf\":" << self->_center_freq.load() << ",\"bins\":[";

      std::vector<float> db_copy;
      {
        std::lock_guard lock(self->_frame_mutex);
        db_copy = self->_pending_db;
      }

      for (int i = 0; i < static_cast<int>(db_copy.size()); ++i) {
        if (i > 0) json << ',';
        // Round to 1 decimal place to reduce the bandwidth
        json << static_cast<int>(db_copy[i] * 10) / 10.0f;
      }
      json << "]}";

      std::string msg = json.str();

      // libwebsockets requires LWS_PRE bytes of padding before the payload
      std::vector<uint8_t> buf(LWS_PRE + msg.size());
      std::memcpy(buf.data() + LWS_PRE, msg.data(), msg.size());

      lws_write(wsi, buf.data() + LWS_PRE, msg.size(), LWS_WRITE_TEXT);

      session->pending = false;

      break;
    }

    case LWS_CALLBACK_EVENT_WAIT_CANCELLED: {
      // broadcast() was called - mark all sessions as pending
      // and request writeable callbacks
      bool ready;
      {
        std::lock_guard lock(self->_frame_mutex);
        ready = self->_frame_ready;
        self->_frame_ready = false;
      }
      if (ready) {
        // request write callback for all connected clients
        lws_callback_on_writable_all_protocol(lws_get_context(wsi),
                                              lws_get_protocol(wsi));
      }
      break;
    }

    case LWS_CALLBACK_HTTP: {
        // Read and serve index.html
        const std::string path = "src/ui/www/index.html";
        std::ifstream file(path);
        if (!file.is_open()) {
            lws_return_http_status(wsi, HTTP_STATUS_NOT_FOUND, nullptr);
            return -1;
        }
        std::string html((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());

        std::string headers =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: " + std::to_string(html.size()) + "\r\n"
            "Connection: close\r\n\r\n";

        std::string response = headers + html;
        std::vector<uint8_t> buf(LWS_PRE + response.size());
        std::memcpy(buf.data() + LWS_PRE, response.data(), response.size());
        lws_write(wsi, buf.data() + LWS_PRE, response.size(), LWS_WRITE_HTTP);
        return -1;
    }

    default:
      break;
  }

  return 0;
}
