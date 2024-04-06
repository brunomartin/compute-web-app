#ifndef CWA_DATA_DATA_AGGREGATOR_H
#define CWA_DATA_DATA_AGGREGATOR_H

#include <functional>

#include "data/data.h"
#include "data/dataset.h"
#include "data/dataset_aggregator_server.h"

namespace cwa {
namespace data {

class DataAggregator {
public:
  DataAggregator(DatasetAggregatorServerPtr dataset_aggregator);
  
  int GetId() const;

  /**
   Start pulling datasets to data
   @param data data where to aggregate pulled datasets
   */
  void Start(DataPtr data);
  
  
  typedef std::function<void(DatasetPtr)> DatasetPulledCallback;
  void SetDatasetPulledCallback(DatasetPulledCallback callback);
  
  /**
   Wait until all dataset have been pulled
   */
  void Wait();
  
private:
  class Private;
  std::shared_ptr<Private> data_;
  
};

}
}

#endif // CWA_DATA_DATA_AGGREGATOR_H
