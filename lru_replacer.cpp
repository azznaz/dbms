#include "lru_replacer.h"

namespace dbms {

LRUReplacer::LRUReplacer(size_t num_pages) { max_num_pages_ = num_pages; }

LRUReplacer::~LRUReplacer() = default;

bool LRUReplacer::Victim(frame_id_t *frame_id) {
  std::lock_guard lock(latch_);
  if(lru_.empty()) {
    return false;
  }
  *frame_id = lru_.front();
  mp_.erase(*frame_id);
  lru_.pop_front();
  return true;
}

void LRUReplacer::Pin(frame_id_t frame_id) {
  std::lock_guard lock(latch_);
  if(mp_.count(frame_id) != 0) {
    lru_.erase(mp_[frame_id]);
    mp_.erase(frame_id);
  }     
}

void LRUReplacer::Unpin(frame_id_t frame_id) {
  std::lock_guard lock(latch_);
  if(mp_.count(frame_id) == 0) {
    lru_.push_back(frame_id);
    auto iter = lru_.end();
    iter--;
    mp_[frame_id] = iter;
  }
}

size_t LRUReplacer::Size() {
  std::lock_guard lock(latch_);
  return lru_.size();
}

}