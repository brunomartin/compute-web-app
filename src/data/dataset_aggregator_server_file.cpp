#include "data/dataset_aggregator_server_file.h"

#include <set>
#include <queue>
#include <string>
#include <thread>
#include <chrono>

#include <log/log.h>

#include "data/utils.h"
#include "data/data.h"
#include "data/data_exception.h"
#include "data/dataset.h"
#include "data/dataset_exchange_file.h"

using cwa::Log;
using cwa::LogLevel;

using std::chrono::high_resolution_clock;

namespace cwa {
namespace data {

class DatasetAggregatorServerFile::Private {
public:
      
  std::unique_ptr<DatasetExchangeFile> exchange_file;
  
  int id;
  bool is_server;
  
  std::string buffer;
};

DatasetAggregatorServerFile::DatasetAggregatorServerFile() : data_(new Private) {
  data_->id = GetId();
}

DatasetAggregatorServerFile::~DatasetAggregatorServerFile() {
  Close();
}

void DatasetAggregatorServerFile::Bind(const std::string & info) {
  
  data_->exchange_file.reset(new DatasetExchangeFile(info));
  
  // In case previous run has failed, delete files
  data_->exchange_file->RemoveFiles();
  
  data_->exchange_file->LockFile();
  data_->exchange_file->Init(true);
  data_->exchange_file->UnlockFile();
  
  Log::Print(LogLevel::Info, "[Agg %d][file] Exchange file created", GetId());
}

void DatasetAggregatorServerFile::Close() {
  if(data_->exchange_file) {
    data_->exchange_file->RemoveFiles();    
  }
}

std::string DatasetAggregatorServerFile::GetInfo() const {
  return data_->exchange_file->GetFilename();
}

void DatasetAggregatorServerFile::PullDataset(DatasetPtr& dataset, uint32_t & dataset_id) {
  
  uint32_t length = -1;
  std::string filename;

  Log::Print(LogLevel::Debug, "[Agg %d][file] Pulling dataset from server...", GetId());
    
  try {
    
    // Server may have not been started so wait until there is dataset
    // in the queue
    while(true) {
      data_->exchange_file->LockFile();
      if(data_->exchange_file->HasDataset()) break;
      data_->exchange_file->UnlockFile();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Get dataset id here
    dataset_id = data_->exchange_file->GetFirstDataset().id;
        
    // Update exchange file only if not end signal dataset, this part is
    // handled in PushEndSignal()
    // File needs to be locked for consistency, it has to be unlocked
    // in PushEndSignal()
    if(dataset_id == -1) {
      return;
    }
    
    // if end dataset, there is no filename so it can be retrieved
    // safely here
    filename = data_->exchange_file->GetFirstDataset().filename;
    
    // If dataset is ok, retrieve it
    
    // remove dataset from the queue to tell aggregator has handle it
    data_->exchange_file->PopFirstDataset();
    
    data_->exchange_file->UnlockFile();
    
    Log::Print(LogLevel::Debug, "[Agg %d][file] Pulling dataset %d from server...", GetId(), dataset_id);
    
    // In case dataset has not been allocated yet, do it now
    if(!dataset) dataset.reset(new Dataset);
    
    // Read from filename and store buffer in dataset
    dataset->RetrieveBufferFromFile(filename);
    
    // Now we have the id, retrieve buffer from dataset
    dataset->SetId("dataset_" + std::to_string(dataset_id));
        
    Log::Print(LogLevel::Debug, "[Agg %d][file] Pulled dataset %d from server", GetId(), dataset_id);
    
  } catch(CannotConnectToServerException&) {
    Log::Print(LogLevel::Error, "[Agg %d][file] Could not connect to server", GetId());
  } catch (std::exception& e) {
    Log::Print(LogLevel::Error, "[Agg %d][file] Exception: %s", GetId(), e.what());
    exit(-1);
  }
    
}

void DatasetAggregatorServerFile::HandleEndSignal() {
  
  // As a server, receiving -1 dataset tell client
  // is unregistering
  
  // File is locked when arriving here
  // Get the client of the last dataset detected
  uint32_t client_id = data_->exchange_file->GetFirstDataset().client_id;
  
  // Remove it from the registered clients
  data_->exchange_file->UnregisterClient(client_id);
  data_->exchange_file->PopFirstDataset();
  
  data_->exchange_file->UnlockFile();
  
  Log::Print(LogLevel::Info, "[Agg %d][file] client %d unregistered", GetId(), client_id);
          
  // Check if last client unregistered, in that case, aggregator
  // can be safely closed
  
  bool last_client = data_->exchange_file->AreAllClientsUnregistered();
  
  if(last_client) {
    Log::Print(LogLevel::Info, "[Agg %d][file] No more supplier", GetId());
    throw NoMoreSupplierException();
  }
  
}
  
}
}
