#ifndef CWA_PROCESS_WORKER_MULTI_THREAD_H
#define CWA_PROCESS_WORKER_MULTI_THREAD_H

#include <memory>
#include <string>
#include <functional>

#include <data/dataset.h>

#include "process/worker.h"

using cwa::data::DatasetPtr;
using cwa::data::ConstDatasetPtr;

namespace cwa {
namespace process {

class WorkerMultiThread : public Worker {
public:
  
  typedef std::function<void(const std::string&, const std::string&, const std::string&, const std::string&)> RunFunction;
  
  WorkerMultiThread(const Definition & definition, const VariantMap & parameter_values,
                    DatasetSupplierServerPtr supplier, DatasetAggregatorServerPtr aggregator,
                    RunFunction run_function);
  
  ~WorkerMultiThread();
  
  virtual void Launch() override;
  
private:
  class Private;
  std::shared_ptr<Private> data_;
};

}

}

#endif // CWA_PROCESS_WORKER_MULTI_THREAD_H
