#ifndef CWA_DATA_DATA_SUPPLIER_H
#define CWA_DATA_DATA_SUPPLIER_H

#include "data/dataset_supplier_server.h"
#include "data/data_supply_state.h"

namespace cwa {
namespace data {

class DataSupplier {
public:
  DataSupplier(DatasetSupplierServerPtr dataset_supplier);
    
  int GetId() const;
  
  /**
   Start pushing dataset from data
   */
  void Start(DataSupplyStatePtr supply_state);
  
  /**
   Wait until all dataset have been pushed
   */
  void Wait();
  
private:
  class Private;
  std::shared_ptr<Private> data_;
  
};

typedef std::shared_ptr<DataSupplier> DataSupplierPtr;

}
}

#endif // CWA_DATA_SUPPLIER_H
