#include "data/data_supplier.h"

#include <thread>

#include <log/log.h>

#include "data/data_exception.h"
#include "data/dataset_supplier.h"

using cwa::Log;
using cwa::LogLevel;

namespace cwa {
namespace data {

class DataSupplier::Private {
public:
  std::thread thread;
  DatasetSupplierServerPtr dataset_supplier;
  
  void Run(DataSupplyStatePtr supply_state);
  
  int id;
  static int next_id;
  int total_buffers = 0;
};

int DataSupplier::Private::next_id = 1;

void DataSupplier::Private::Run(DataSupplyStatePtr supply_state) {
  
  Log::Print(LogLevel::Info, "[Data Sup %d] started", id);
  
  while(true) {
    try {
      ConstDatasetPtr dataset;
      uint32_t dataset_id;
      supply_state->GrabDataset(dataset, dataset_id);
      
      Log::Print(LogLevel::Debug, "[Data Sup %d] Pushing dataset %d...", id, dataset_id);
      dataset_supplier->PushDataset(dataset, dataset_id);
      Log::Print(LogLevel::Debug, "[Data Sup %d] Pushed dataset %d", id, dataset_id);
      
      total_buffers++;
      
    } catch (NoMoreDatasetException) {
      break;
    }
  }
  
  Log::Print(LogLevel::Info, "[Data Sup %d] Total datasets pushed: %d", id, total_buffers);
  
  dataset_supplier->PushEndSignal();
}

DataSupplier::DataSupplier(DatasetSupplierServerPtr dataset_supplier) : data_(new Private) {
  data_->dataset_supplier = dataset_supplier;
  data_->id = data_->next_id;
  data_->next_id++;  
}

int DataSupplier::GetId() const {
  return data_->id;
}

void DataSupplier::Start(DataSupplyStatePtr supply_state) {
  data_->thread = std::thread(&Private::Run, data_, supply_state);
}

void DataSupplier::Wait() {
  if(data_->thread.joinable()) data_->thread.join();
}
  
}
}
