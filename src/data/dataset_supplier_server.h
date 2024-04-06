#ifndef CWA_DATA_DATASET_SUPPLIER_SERVER_H
#define CWA_DATA_DATASET_SUPPLIER_SERVER_H

#include "data/dataset_supplier.h"

namespace cwa {
namespace data {

class DatasetSupplierServer : public DatasetSupplier {
public:
  virtual void Bind(const std::string & info) = 0;
  virtual std::string GetInfo() const = 0;
  
private:
};

typedef std::shared_ptr<DatasetSupplierServer> DatasetSupplierServerPtr;

}
}

#endif // CWA_DATA_DATASET_SUPPLIER_SERVER_H
