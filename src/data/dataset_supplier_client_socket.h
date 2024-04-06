#ifndef CWA_DATA_DATASET_SUPPLIER_CLIENT_SOCKET_H
#define CWA_DATA_DATASET_SUPPLIER_CLIENT_SOCKET_H

#include "data/dataset_supplier_client.h"

namespace cwa {
namespace data {

class DatasetSupplierClientSocket : public DatasetSupplierClient {
public:
  DatasetSupplierClientSocket();
  ~DatasetSupplierClientSocket();
  
  virtual void Connect(const std::string & info) override;
  virtual void Close() override;
  
  virtual void PushDataset(ConstDatasetPtr dataset, uint32_t index) override;
  virtual void PushEndSignal() override;

private:
  class Private;
  std::unique_ptr<Private> data_;
};

}
}

#endif // CWA_DATA_DATASET_SUPPLIER_CLIENT_SOCKET_H
