#ifndef CWA_DATA_DATASET_AGGREGATOR_SERVER_H
#define CWA_DATA_DATASET_AGGREGATOR_SERVER_H

#include "data/dataset_aggregator.h"

namespace cwa {
namespace data {

class DatasetAggregatorServer : public DatasetAggregator {
public:
  virtual void Bind(const std::string & info) = 0;
  virtual std::string GetInfo() const = 0;
  
private:
};

typedef std::shared_ptr<DatasetAggregatorServer> DatasetAggregatorServerPtr;

}
}

#endif // CWA_DATA_DATASET_AGGREGATOR_H
