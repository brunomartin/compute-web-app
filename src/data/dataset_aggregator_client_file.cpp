#include "data/dataset_aggregator_client_file.h"

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

class DatasetAggregatorClientFile::Private {
public:
      
  std::unique_ptr<DatasetExchangeFile> exchange_file;
  
  int id;
  
  std::string buffer;
};

DatasetAggregatorClientFile::DatasetAggregatorClientFile() : data_(new Private) {
  data_->id = GetId();
}

DatasetAggregatorClientFile::~DatasetAggregatorClientFile() {
  
}

void DatasetAggregatorClientFile::Connect(const std::string & info) {
  
  data_->exchange_file.reset(new DatasetExchangeFile(info));
  
  try {
    data_->exchange_file->WaitForFile(GetTimeout());
  } catch (TimeoutException&) {
    throw CannotConnectToServerException();
  }
  
  Log::Print(LogLevel::Info, "[Agg %d][file] Registered as client", GetId());
}

void DatasetAggregatorClientFile::Close() {
  
}

void DatasetAggregatorClientFile::PullDataset(DatasetPtr &dataset, uint32_t & dataset_id) {
  
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
    
    // remove dataset from the queue to tell
    // aggregator has handle it
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

void DatasetAggregatorClientFile::HandleEndSignal() {  
  data_->exchange_file->UnlockFile();
  Log::Print(LogLevel::Info, "[Agg %d][file] Unregistered from server as client", GetId());
}
  
}
}
