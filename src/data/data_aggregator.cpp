#include "data/data_aggregator.h"

#include <thread>

#include <log/log.h>

#include "data/data.h"
#include "data/data_exception.h"
#include "data/dataset.h"

using cwa::Log;
using cwa::LogLevel;

namespace cwa {
namespace data {

class DataAggregator::Private {
public:
  DatasetAggregatorServerPtr dataset_aggregator;
  std::thread thread;
  
  DatasetPulledCallback on_dataset_pulled;
  
  void Run(DataPtr data);
  
  int id;
  static int next_id;
  int total_buffers = 0;
};

int DataAggregator::Private::next_id = 1;

void DataAggregator::Private::Run(DataPtr data) {
  
    Log::Print(LogLevel::Info, "[Data Agg %d] started", id);
        
    while(true) {
      DatasetPtr dataset(new Dataset);
      
      uint32_t dataset_id = 0;
      
      // Dataset need to know its data for looking in a directory
      // for instance
      dataset->SetData(data.get());
      
      Log::Print(LogLevel::Debug, "[Data Agg %d] Pulling dataset...", id);
      
      // Pull dataset using current aggregator index is updated with
      // pulled dataset, dataset shall contain a buffer in memory at this level
      dataset_aggregator->PullDataset(dataset, dataset_id);
      
      // When index is -1, in case aggregator is a server, connected suppliers
      // tell this aggregator that it has no more data. dataset aggregator shall
      // return information about still connected supplier. In case there is no
      // more dataset supplier connected, we may leave the loop.
      if(dataset_id == -1) {
        try {
          dataset_aggregator->HandleEndSignal();
          continue;
        } catch(NoMoreSupplierException &) {
          break;
        }
      }
      
      total_buffers++;
      
      Log::Print(LogLevel::Debug, "[Data Agg %d] Pulled dataset %d", id, dataset_id);
      data->AddDataset(dataset);
      
      // If a callback has been defined, it is executed here after pulling
      // a dataset. If no callback is defined, dataset content will be kept
      // in memory
      if(on_dataset_pulled) {
        on_dataset_pulled(dataset);
      }
    }
    
    Log::Print(LogLevel::Info, "[Agg %d] Total datasets pulled: %d", id, total_buffers);
}

DataAggregator::DataAggregator(DatasetAggregatorServerPtr dataset_aggregator) :
  data_(new Private) {
  data_->dataset_aggregator = dataset_aggregator;
  data_->id = data_->next_id;
  data_->next_id++;
}

int DataAggregator::GetId() const {
  return data_->id;
}

void DataAggregator::Start(DataPtr data) {
  data_->thread = std::thread(&Private::Run, data_, data);
}

void DataAggregator::SetDatasetPulledCallback(DatasetPulledCallback callback) {
  data_->on_dataset_pulled = callback;
}

void DataAggregator::Wait() {
  if(data_->thread.joinable()) data_->thread.join();
}
  
}
}
