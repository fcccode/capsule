
#include <capsule.h>

#include <chrono>
#include <mutex>

#define FPS 60
static auto frame_interval = std::chrono::microseconds(1000000 / FPS);

static bool first_frame = true;
static int frame_count = 0;
static std::chrono::time_point<std::chrono::steady_clock> first_ts;

using namespace std;

mutex capdata_mutex;

bool capsule_capture_active () {
  lock_guard<mutex> lock(capdata_mutex);
  return capdata.active;
}

void capsule_capture_flip () {
  lock_guard<mutex> lock(capdata_mutex);
  capdata.active = !capdata.active;
}

static inline bool capsule_frame_ready () {
  static std::chrono::time_point<std::chrono::steady_clock> last_ts;

  auto interval = frame_interval;

  if (first_frame) {
    first_frame = false;
    first_ts = std::chrono::steady_clock::now();
    last_ts = first_ts;
    return false;
  }

  auto t = std::chrono::steady_clock::now();
  auto elapsed = t - last_ts;

  if (elapsed < interval) {
    return false;
  }

  // logic taken from libobs  
  bool dragging = (elapsed > (interval * 2));
  if (dragging) {
    last_ts = t;
  } else {
    last_ts = last_ts + interval;
  }
  return true;
}

int64_t CAPSULE_STDCALL capsule_frame_timestamp () {
  auto frame_timestamp = std::chrono::steady_clock::now() - first_ts;
  auto micro_timestamp = std::chrono::duration_cast<std::chrono::microseconds>(frame_timestamp);
  return (int64_t) micro_timestamp.count();
}

bool CAPSULE_STDCALL capsule_capture_ready () {
  return capsule_capture_active() && capsule_frame_ready();
}
