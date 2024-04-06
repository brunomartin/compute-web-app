#ifndef CWA_DATA_DATASET_SUPPLIER_CLIENT_FILE_H
#define CWA_DATA_DATASET_SUPPLIER_CLIENT_FILE_H

#include "data/dataset_supplier_client.h"

namespace cwa {
namespace data {

class DatasetSupplierClientFile : public DatasetSupplierClient {
public:
  DatasetSupplierClientFile();
  ~DatasetSupplierClientFile();
  
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

#endif // CWA_DATA_DATASET_SUPPLIER_FILE_H
