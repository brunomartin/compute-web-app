#ifndef CWA_PROCESS_WORKER_H
#define CWA_PROCESS_WORKER_H

#include <memory>
#include <string>

#include <data/dataset_supplier_server.h>
#include <data/dataset_aggregator_server.h>

#include "process/definition.h"

using cwa::data::DatasetSupplierServerPtr;
using cwa::data::DatasetAggregatorServerPtr;

namespace cwa {
namespace process {

class WorkerPrivate;

class Worker {
public:
  Worker(const Definition & definition, const VariantMap & parameter_values,
    DatasetSupplierServerPtr supplier, DatasetAggregatorServerPtr aggregator);
    
  virtual void Launch() = 0;
  
protected:
  const Definition & GetDefinition();
  const VariantMap & GetParameterValues();
  DatasetSupplierServerPtr GetSupplier();
  DatasetAggregatorServerPtr GetAggregator();
  
private:
  class Private;
  std::shared_ptr<Private> data_;
  
};

typedef std::shared_ptr<Worker> WorkerPtr;

}
}

#endif // CWA_PROCESS_WORKER_H
