#ifndef CWA_DATA_DATASET_SUPPLIER_CLIENT_H
#define CWA_DATA_DATASET_SUPPLIER_CLIENT_H

#include "data/dataset_supplier.h"

namespace cwa {
namespace data {

class DatasetSupplierClient : public DatasetSupplier {
public:
  virtual void Connect(const std::string & info) = 0;
  
private:
};

typedef std::shared_ptr<DatasetSupplierClient> DatasetSupplierClientPtr;

}
}

#endif // CWA_DATA_DATASET_SUPPLIER_CLIENT_H
