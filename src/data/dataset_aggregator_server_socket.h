#ifndef CWA_DATA_DATASET_AGGREGATOR_SERVER_SOCKET_H
#define CWA_DATA_DATASET_AGGREGATOR_SERVER_SOCKET_H

#include "data/dataset_aggregator_server.h"

namespace cwa {
namespace data {

class DatasetAggregatorServerSocket : public DatasetAggregatorServer {
public:
  DatasetAggregatorServerSocket();
  ~DatasetAggregatorServerSocket();
  
  virtual void Bind(const std::string & info) override;
  virtual void Close() override;
  
  virtual std::string GetInfo() const override;
  virtual void PullDataset(DatasetPtr& dataset, uint32_t & index) override;
  virtual void HandleEndSignal() override;

private:
  class Private;
  std::unique_ptr<Private> data_;
};

}
}

#endif // CWA_DATA_DATASET_AGGREGATOR_SERVER_SOCKET_H
