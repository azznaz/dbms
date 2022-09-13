#include <sys/stat.h>
#include <cassert>
#include <cstring>
#include <iostream>
#include <mutex>  // NOLINT
#include <string>
#include <thread>  // NOLINT

#include "disk_manager.h"
#include "common/exception.h"
#include "common/logger.h"
namespace dbms{
    static char *buffer_used;

DiskManager::DiskManager(const std::string &db_file)
    : file_name_(db_file), num_writes_(0) {
        std::string::size_type n = db_file.rfind('.');
        if(n == std::string::npos){
            LOG_DEBUG("wrong file format");
            return;
        }
        std::lock_guard lock(db_io_latch_);
        db_io_.open(db_file,std::ios::binary | std::ios::in | std::ios::out);
        if(!db_io_.is_open()){
            db_io_.open(db_file,std::ios::binary | std::ios::trunc | std::ios::app | std::ios::out);
            db_io_.close();
            db_io_.open(db_file,std::ios::binary | std::ios::in | std::ios::out);
            if(!db_io_.is_open()){
                throw Exception("can't open db file");
            }
        }
        buffer_used = nullptr;
    }

void DiskManager::ShutDown(){
    {
        std::lock_guard lock(db_io_latch_);
        db_io_.close();
    }
}

void DiskManager::WritePage(page_id_t page_id,const char *page_data){
    std::lock_guard lock(db_io_latch_);
    size_t offset = page_id * PAGE_SIZE;
    db_io_.seekp(offset);
    db_io_.write(page_data,PAGE_SIZE);
    num_writes_ += 1;
    if(db_io_.bad()){
        LOG_DEBUG("I/O error while writing");
        return;
    }
    db_io_.flush();
}

void DiskManager::ReadPage(page_id_t page_id,char *page_data){
    std::lock_guard lock(db_io_latch_);
    size_t offset = page_id * PAGE_SIZE;
    if(offset > GetFileSize(file_name_)){
        LOG_DEBUG("I/O error reading past end of file");
    }else{
        db_io_.seekp(offset);
        db_io_.read(page_data,PAGE_SIZE);
        if(db_io_.bad()){
            LOG_DEBUG("I/O error while reading");
            return;
        }
        int read_count = db_io_.gcount();
        if(read_count < PAGE_SIZE){
            LOG_DEBUG("Read less than a page");
            db_io_.clear();
            memset(page_data + read_count, 0, PAGE_SIZE - read_count);
        }
    }
}

int DiskManager::GetFileSize(const std::string &file_name) {
  struct stat stat_buf;
  int rc = stat(file_name.c_str(), &stat_buf);
  return rc == 0 ? static_cast<int>(stat_buf.st_size) : -1;
}

}