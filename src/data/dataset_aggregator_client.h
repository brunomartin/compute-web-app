#ifndef CWA_DATA_DATASET_AGGREGATOR_CLIENT_H
#define CWA_DATA_DATASET_AGGREGATOR_CLIENT_H

#include "data/dataset_aggregator.h"

namespace cwa {
namespace data {

class DatasetAggregatorClient : public DatasetAggregator {
public:  
  virtual void Connect(const std::string & info) = 0;
  
private:
};

typedef std::shared_ptr<DatasetAggregatorClient> DatasetAggregatorClientPtr;

}
}

#endif // CWA_DATA_DATASET_AGGREGATOR_CLIENT_H
