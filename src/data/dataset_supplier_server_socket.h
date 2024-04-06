#ifndef CWA_DATA_DATASET_SUPPLIER_SERVER_SOCKET_H
#define CWA_DATA_DATASET_SUPPLIER_SERVER_SOCKET_H

#include "data/dataset_supplier_server.h"

namespace cwa {
namespace data {

class DatasetSupplierServerSocket : public DatasetSupplierServer {
public:
  DatasetSupplierServerSocket();
  ~DatasetSupplierServerSocket();
  
  virtual void Bind(const std::string & info) override;
  virtual void Close() override;
  
  virtual std::string GetInfo() const override;
  virtual void PushDataset(ConstDatasetPtr dataset, uint32_t index) override;
  virtual void PushEndSignal() override;

private:
  class Private;
  std::unique_ptr<Private> data_;
};

}
}

#endif // CWA_DATA_DATASET_SUPPLIER_SERVER_SOCKET_H
