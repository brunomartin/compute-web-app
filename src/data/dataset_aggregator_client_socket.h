#ifndef CWA_DATA_DATASET_AGGREGATOR_CLIENT_SOCKET_H
#define CWA_DATA_DATASET_AGGREGATOR_CLIENT_SOCKET_H

#include "data/dataset_aggregator_client.h"

namespace cwa {
namespace data {

class DatasetAggregatorClientSocket : public DatasetAggregatorClient {
public:
  DatasetAggregatorClientSocket();
  ~DatasetAggregatorClientSocket();
  
  virtual void Connect(const std::string & info) override;
  virtual void Close() override;
  
  virtual void PullDataset(DatasetPtr& dataset, uint32_t & index) override;
  virtual void HandleEndSignal() override;

private:
  class Private;
  std::unique_ptr<Private> data_;
};

}
}

#endif // CWA_DATA_DATASET_AGGREGATOR_CLIENT_SOCKET_H
