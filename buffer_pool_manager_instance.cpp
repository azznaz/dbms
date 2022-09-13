#include "buffer_pool_manager_instance.h"
#include <fstream>
#include "common/macros.h"

namespace dbms{

BufferPoolManagerInstance::BufferPoolManagerInstance(size_t pool_size, DiskManager *disk_manager)
    : pool_size_(pool_size), disk_manager_(disk_manager) {}

BufferPoolManagerInstance::BufferPoolManagerInstance(size_t pool_size, uint32_t num_instances, uint32_t instance_index,
                            DiskManager *disk_manager)
    : pool_size_(pool_size),
      num_instances_(num_instances),
      instance_index_(instance_index),
      disk_manager_(disk_manager) {
    BUSTUB_ASSERT(num_instances > 0, "If BPI is not part of a pool, then the pool size should just be 1");
    BUSTUB_ASSERT(
      instance_index < num_instances,
      "BPI index cannot be greater than the number of BPIs in the pool. In non-parallel case, index should just be 1.");    
    std::lock_guard lock(latch_);
    pages_ = new Page[pool_size];
    replacer_ = nullptr;
    for(int i = 0; i < pool_size; i++) {
        free_list_.push_back(i);
    }
}        
BufferPoolManagerInstance::~BufferPoolManagerInstance() {
  std::lock_guard lock(latch_);
  delete []pages_;
  delete replacer_;
}

void BufferPoolManagerInstance::Flush(frame_id_t frame_id) {
  Page *page = &(pages_[frame_id]);
  if(page->IsDirty()){
   disk_manager_->WritePage(page->GetPageId(),page->GetData());
  }
  page->is_dirty_ = false;
}

void BufferPoolManagerInstance::InitPage(Page *page) {
  page->is_dirty_ = false;
  page->ResetMemory();
  page->page_id_ = INVALID_PAGE_ID;
  page->pin_count_ = 0;
}

bool BufferPoolManagerInstance::GetAvailFrame(frame_id_t *res) {
  frame_id_t frame_id = -1;
  if (!free_list_.empty()) {
    frame_id = free_list_.front();
    free_list_.pop_front();
  }

  if (frame_id == -1) {
    if(replacer_->Victim(&frame_id) == false) {
        return false;
    }
  }
  *res = frame_id;
  return true;
}

bool BufferPoolManagerInstance::FlushPgImp(page_id_t page_id) {
  std::lock_guard lock(latch_);
  if(page_table_.count(page_id) == 0){
     return false;
  }
  frame_id_t frame_id = page_table_[page_id];
  Flush(frame_id);
  return true;
}
void BufferPoolManagerInstance::FlushAllPgsImp() {
  std::lock_guard lock(latch_);
  for(const auto &page_pair : page_table_){
     frame_id_t frame_id = page_pair.second;
     Flush(frame_id);
  }
}
Page *BufferPoolManagerInstance::NewPgImp(page_id_t *page_id) {
  std::lock_guard lock(latch_);
  frame_id_t frame_id = -1;
  if(!GetAvailFrame(&frame_id)){
    return nullptr;
  }
  Flush(frame_id);
  *page_id = AllocatePage();
  Page *page = &(pages_[frame_id]);
  InitPage(page);
  page_table_.erase(page->GetPageId());
  page->page_id_ = *page_id;
  page->pin_count_ = 1;
  page_table_[*page_id] = frame_id;
  replacer_->Pin(frame_id);
  disk_manager_->WritePage(page->GetPageId(),page->GetData());
}

bool BufferPoolManagerInstance::DeletePgImp(page_id_t page_id) {
  std::lock_guard lock(latch_);
  if (page_table_.count(page_id) == 0) {
    DeallocatePage(page_id);
    return true;
  }
  frame_id_t frame_id = page_table_[page_id];
  Page *page = &pages_[frame_id];
  if (page->pin_count_ > 0) {
    return false;
  }
  DeallocatePage(page_id);
  Flush(frame_id);
  InitPage(page);
  page_table_.erase(page_id);
  free_list_.push_back(frame_id);
  return true;
}

Page *BufferPoolManagerInstance::FetchPgImp(page_id_t page_id) {
  std::lock_guard lock(latch_);
  if (page_table_.count(page_id) != 0) {
    Page *p = &pages_[page_table_[page_id]];
    p->pin_count_++;
    replacer_->Pin(page_table_[page_id]);
    return p;
  }
  frame_id_t frame_id = -1;
  if(!GetAvailFrame(&frame_id)){
    return nullptr;
  }
  Flush(frame_id);
  Page *page = &(pages_[frame_id]);
  InitPage(page);
  page_table_.erase(page->GetPageId());
  page->page_id_ = page_id;
  page->pin_count_ = 1;
  page_table_[page_id] = frame_id;
  replacer_->Pin(frame_id);
  disk_manager_->ReadPage(page->GetPageId(),page->GetData());
}

bool BufferPoolManagerInstance::UnpinPgImp(page_id_t page_id, bool is_dirty) {
  std::lock_guard lock(latch_);
  if (page_table_.count(page_id) == 0) {
    return true;
  }
  frame_id_t frame_id = page_table_[page_id];
  Page *page = &(pages_[frame_id]);
  if(is_dirty){
    page->is_dirty_ = is_dirty;
  }
  if (page->GetPinCount() <= 0) {
    return false;
  }
  page->pin_count_ -= 1; 

  if(page->GetPinCount() == 0){
    replacer_->Unpin(frame_id);
  }
  return true;
}

page_id_t BufferPoolManagerInstance::AllocatePage() {
  const page_id_t next_page_id = next_page_id_;
  next_page_id_ += num_instances_;
  ValidatePageId(next_page_id);
  return next_page_id;
}

void BufferPoolManagerInstance::ValidatePageId(const page_id_t page_id) const {
  assert(page_id % num_instances_ == instance_index_);  // allocated pages mod back to this BPI
}

} 

