#include <atomic>
#include <fstream>
#include <future>  // NOLINT
#include <mutex>   // NOLINT
#include <string>
#include "common/config.h"

namespace dbms{
  class DiskManager{
    public:
     explicit DiskManager(const std::string &db_file);
    
     ~DiskManager() = default;

     void ShutDown();

     void WritePage(page_id_t page_id,const char *page_data);

     void ReadPage(page_id_t page_id,char *page_data);

     int GetNumWrites() const;

    private:
     int GetFileSize(const std::string &file_name);
     
     std::fstream db_io_;
     std::string file_name_;
     int num_writes_;
     
     std::mutex db_io_latch_;
  };
}