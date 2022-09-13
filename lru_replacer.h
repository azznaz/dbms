#include <list>
#include <mutex>  // NOLINT
#include <unordered_map>
#include <vector>
#include "replacer.h"
#include "common/config.h"

namespace dbms{

class LRUReplacer : Replacer {
 public:
  explicit LRUReplacer(size_t num_pages);

  ~LRUReplacer() override;

  bool Victim(frame_id_t *frame_id) override;

  void Pin(frame_id_t frame_id) override;

  void Unpin(frame_id_t frame_id) override;

  size_t Size() override;

 private:
  std::mutex latch_;
  size_t max_num_pages_;
  std::unordered_map<frame_id_t, std::list<frame_id_t>::iterator> mp_;
  std::list<frame_id_t> lru_;
};
}