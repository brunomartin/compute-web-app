#ifndef CWA_DATA_DATASET_AGGREGATOR_H
#define CWA_DATA_DATASET_AGGREGATOR_H

#include "data/dataset.h"

namespace cwa {
namespace data {

class DatasetAggregator {
public:
  DatasetAggregator();
  virtual ~DatasetAggregator() {};
  
  int GetId() const;
  
  void SetTimeout(int timeout_ms);
  int GetTimeout() const;
  
  virtual void Close() = 0;
  
  virtual void PullDataset(DatasetPtr& dataset, uint32_t & index) = 0;
  virtual void HandleEndSignal() = 0;
  
private:
  class Private;
  std::shared_ptr<Private> data_;
};

typedef std::shared_ptr<DatasetAggregator> DatasetAggregatorPtr;

}
}

#endif // CWA_DATA_DATASET_AGGREGATOR_H
