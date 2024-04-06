#include "data/dataset_supplier_client_file.h"

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

class DatasetSupplierClientFile::Private {
public:
    
  std::unique_ptr<DatasetExchangeFile> exchange_file;
  
  int id;
  uint32_t client_id = -1;
};

DatasetSupplierClientFile::DatasetSupplierClientFile() : data_(new Private) {
  data_->id = GetId();
}

DatasetSupplierClientFile::~DatasetSupplierClientFile() {
  
}

void DatasetSupplierClientFile::Connect(const std::string & info) {
  data_->exchange_file.reset(new DatasetExchangeFile(info));
  
  try {
    data_->exchange_file->WaitForFile(GetTimeout());
  } catch (TimeoutException&) {
    throw CannotConnectToServerException();
  }
  
  data_->exchange_file->LockFile();
  data_->client_id = data_->exchange_file->RegisterClient();
  data_->exchange_file->UnlockFile();
  
  Log::Print(LogLevel::Info, "[Sup %d][file] Registered as client %d", GetId(), data_->client_id);
}

void DatasetSupplierClientFile::Close() {
  
}

void DatasetSupplierClientFile::PushDataset(ConstDatasetPtr dataset, uint32_t dataset_id) {
    
  try {
    // Add dataset only if not end signal dataset, this part is handle in
    // PushEndSignal()
    if(dataset_id == -1) {
      return;
    }
    
    DatasetExchangeFile::Dataset exchange_dataset;
    exchange_dataset.id = dataset_id;
    exchange_dataset.filename = dataset->GetFilename();
    
    data_->exchange_file->LockFile();
    data_->exchange_file->PushDataset(exchange_dataset);
    data_->exchange_file->UnlockFile();
    
    Log::Print(LogLevel::Debug, "[Sup %d][file] Pushing dataset %d to server as client...", GetId(), dataset_id);
    
  } catch (std::exception& e) {
    Log::Print(LogLevel::Error, "[Sup %d][file] Exception: %s", GetId(), e.what());
    exit(-1);
  }
  
}

void DatasetSupplierClientFile::PushEndSignal() {
  
  DatasetExchangeFile::Dataset exchange_dataset;
  
//  If server, it does not need to know its clients,
//  adding end signal dataset shall be enough
//  In that case, clients must not remove it
  exchange_dataset.id = uint32_t(-1);
  
//  Server needs to know which client is sending
//  the end of dataset signal
  exchange_dataset.client_id = data_->client_id;
  
  data_->exchange_file->LockFile();
  data_->exchange_file->PushDataset(exchange_dataset);
  data_->exchange_file->UnlockFile();

  Log::Print(LogLevel::Info, "[Sup %d][file] Unregistered from server as client %d", GetId(), data_->client_id);
  
}
  
}
}
