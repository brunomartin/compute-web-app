#include "data/dataset_supplier_server_file.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <cstring>

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include <map>
#include <set>
#include <string>
#include <thread>
#include <chrono>

#include <log/log.h>

#include "data/utils.h"
#include "data/data_exception.h"
#include "data/dataset_exchange_file.h"

using cwa::Log;
using cwa::LogLevel;

using std::chrono::high_resolution_clock;

namespace cwa {
namespace data {

class DatasetSupplierServerFile::Private {
public:
    
  std::shared_ptr<DatasetExchangeFile> exchange_file;
  
  int id;
  std::set<uint32_t> client_ids;
  
  // Using mmap may speed up worker file reading up to x10 faster
  // to load files in cache: vmtouch -t mt_ring_input/*
  // to evict files from cache: vmtouch -e mt_ring_input/*
  std::vector<std::thread> read_threads;
};

DatasetSupplierServerFile::DatasetSupplierServerFile() : data_(new Private) {
  data_->id = GetId();
}

DatasetSupplierServerFile::~DatasetSupplierServerFile() {
  Close();
}

void DatasetSupplierServerFile::Bind(const std::string & info) {
  data_->exchange_file.reset(new DatasetExchangeFile(info));

  // In case previous run has failed, delete files
  data_->exchange_file->RemoveFiles();
  
  data_->exchange_file->LockFile();
  // As the server, create the file for exchanging with clients
  data_->exchange_file->Init(false);
  data_->exchange_file->UnlockFile();
  
  Log::Print(LogLevel::Info, "[Sup %d][file] Exchange file created", GetId());
}

void DatasetSupplierServerFile::Close() {
  if(data_->exchange_file) {
    data_->exchange_file->RemoveFiles();
  }
}

std::string DatasetSupplierServerFile::GetInfo() const {
  return data_->exchange_file->GetFilename();
}

void DatasetSupplierServerFile::PushDataset(ConstDatasetPtr dataset, uint32_t dataset_id) {
  
  try {
    // Add dataset only if not end signal dataset, this part is handle in
    // PushEndSignal()
    if(dataset_id == -1) {
      return;
    }
        
    Log::Print(LogLevel::Debug, "[Sup %d][file] Pushing dataset %d to client...", GetId(), dataset_id);
        
    DatasetExchangeFile::Dataset exchange_dataset;
    exchange_dataset.id = dataset_id;
    exchange_dataset.filename = dataset->GetFilename();
    
    auto exchange_file = data_->exchange_file;
    int id = GetId();
    data_->read_threads.push_back(std::thread([exchange_dataset, exchange_file, id, dataset_id] {
      
      // map file to memory to ease aggregator client to read it
      // First get size
      struct stat st;
      if(stat(exchange_dataset.filename.c_str(), &st)) {
        throw Exception("stat file failed");
      }
      
      // Then open file and map it to memory
      int fd = open(exchange_dataset.filename.c_str(), O_RDONLY);
      void* mapped = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
      close(fd);
      
      if(mapped == MAP_FAILED) {
        throw Exception("mmap failed");
      }
      
      // It is needed to read the data so that it is loaded in file system cache
      std::string buffer;
      buffer.resize(st.st_size);
      memcpy((char*)buffer.data(), mapped, st.st_size);
      
      // Once read, we can unmap it
      munmap(mapped, st.st_size);
      
      exchange_file->LockFile();
      exchange_file->PushDataset(exchange_dataset);
      exchange_file->UnlockFile();
      
      Log::Print(LogLevel::Debug, "[Sup %d][file] Dataset %d pushed to client.", id, dataset_id);
      
    }));
    
  } catch (std::exception& e) {
    Log::Print(LogLevel::Error, "[Sup %d][file] Exception: %s", GetId(), e.what());
    exit(-1);
  }
  
}

void DatasetSupplierServerFile::PushEndSignal() {
  
  // Wait here for read threads before sending end signal
  for(auto & thread : data_->read_threads) {
    thread.join();
  }
  
  DatasetExchangeFile::Dataset exchange_dataset;
  
//  As server, it does not need to know its clients,
//  adding end signal dataset shall be enough
//  In that case, clients must not remove it
  exchange_dataset.id = uint32_t(-1);
  
  data_->exchange_file->LockFile();
  data_->exchange_file->PushDataset(exchange_dataset);
  data_->exchange_file->UnlockFile();
  
  Log::Print(LogLevel::Info, "[Sup %d][file] All clients unregistered", GetId());
  
}
  
}
}
