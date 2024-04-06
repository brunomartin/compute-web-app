#ifndef CWA_DATA_DATASET_SUPPLIER_SERVER_FILE_H
#define CWA_DATA_DATASET_SUPPLIER_SERVER_FILE_H

#include "data/dataset_supplier_server.h"

namespace cwa {
namespace data {

class DatasetSupplierServerFile : public DatasetSupplierServer {
public:
  DatasetSupplierServerFile();
  ~DatasetSupplierServerFile();
  
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

#endif // CWA_DATA_DATASET_SUPPLIER_SERVER_FILE_H
