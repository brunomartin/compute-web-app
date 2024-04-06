#ifndef CWA_DATA_DATASET_SUPPLIER_H
#define CWA_DATA_DATASET_SUPPLIER_H

#include "data/dataset.h"

namespace cwa {
namespace data {

class DatasetSupplier {
public:
  DatasetSupplier();
  virtual ~DatasetSupplier() {};
  
  int GetId() const;
  
  void SetTimeout(int timeout_ms);
  int GetTimeout() const;
  
  virtual void Close() = 0;
  
  virtual void PushDataset(ConstDatasetPtr dataset, uint32_t index) = 0;
  virtual void PushEndSignal() = 0;
  
private:  
  class Private;
  std::shared_ptr<Private> data_;
};

typedef std::shared_ptr<DatasetSupplier> DatasetSupplierPtr;

}
}

#endif // CWA_DATA_DATASET_SUPPLIER_H
