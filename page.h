#include <cstring>
#include <iostream>

#include "common/config.h"
#include "common/rwlatch.h"

namespace dbms{

class Page{

  friend class BufferPoolManagerInstance;  
  
  public:
   Page() { ResetMemory(); }

   ~Page() = default;

   inline char *GetData() { return data_; }

   inline page_id_t GetPageId() { return page_id_; }

   inline int GetPinCount() { return pin_count_; }

   inline bool IsDirty() { return is_dirty_; }

   inline void WLatch() { rwlatch_.WLock(); }

   inline void WUnlatch() { rwlatch_.WUnlock(); }

   inline void RLatch() { rwlatch_.RLock(); }

   inline void RUnlatch() { rwlatch_.RUnlock(); }

  protected:
   static constexpr size_t SIZE_PAGE_HEADER = 8;
   static constexpr size_t OFFSET_PAGE_START = 0;
   static constexpr size_t OFFSET_LSN  = 4;
  private:
   inline void ResetMemory() {memset(data_,OFFSET_PAGE_START,PAGE_SIZE);}

   char data_[PAGE_SIZE];
   page_id_t page_id_ = INVALID_PAGE_ID;
   int pin_count_ = 0;
   bool is_dirty_ = false;
   ReaderWriterLatch rwlatch_;
};

}