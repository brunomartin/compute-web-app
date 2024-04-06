#include "worker_multi_thread.h"

#include <thread>

#include <data/data.h>
#include <data/data_exception.h>

#include <data/dataset_supplier_server_file.h>
#include <data/dataset_aggregator_server_file.h>
#include <data/dataset_supplier_server_socket.h>
#include <data/dataset_aggregator_server_socket.h>

#include <data/dataset_supplier_client_file.h>
#include <data/dataset_aggregator_client_file.h>
#include <data/dataset_supplier_client_socket.h>
#include <data/dataset_aggregator_client_socket.h>

using cwa::data::Data;
using cwa::data::DataPtr;
using cwa::data::Dataset;

using cwa::data::DatasetAggregatorClientPtr;
using cwa::data::DatasetSupplierClientPtr;

using cwa::data::DatasetAggregatorServerFile;
using cwa::data::DatasetSupplierServerFile;
using cwa::data::DatasetAggregatorServerSocket;
using cwa::data::DatasetSupplierServerSocket;

using cwa::data::DatasetAggregatorClientFile;
using cwa::data::DatasetSupplierClientFile;
using cwa::data::DatasetAggregatorClientSocket;
using cwa::data::DatasetSupplierClientSocket;

namespace cwa {
namespace process {

class WorkerMultiThread::Private  {
public:
  std::thread thread;
  RunFunction run_function = nullptr;
};

WorkerMultiThread::WorkerMultiThread(const Definition & definition, const VariantMap & parameter_values,
  DatasetSupplierServerPtr supplier, DatasetAggregatorServerPtr aggregator, RunFunction run_function) :
  Worker(definition, parameter_values, supplier, aggregator), data_(new Private) {
  
  data_->run_function = run_function;
    
}

WorkerMultiThread::~WorkerMultiThread() {
  if(data_->thread.joinable()) data_->thread.join();
}


void WorkerMultiThread::Launch() {
  
  std::string sup_info = GetSupplier()->GetInfo();
  std::string agg_info = GetAggregator()->GetInfo();
  
  std::string sup_type;
  if(dynamic_cast<DatasetSupplierServerFile*>(GetSupplier().get())) {
    sup_type = "file";
  } else if(dynamic_cast<DatasetSupplierServerSocket*>(GetSupplier().get())) {
    sup_type = "socket";
  }
  
  std::string agg_type;
  if(dynamic_cast<DatasetAggregatorServerFile*>(GetAggregator().get())) {
    agg_type = "file";
  } else if(dynamic_cast<DatasetAggregatorServerSocket*>(GetAggregator().get())) {
    agg_type = "socket";    
  }
  
  auto run_function = data_->run_function;
  
  data_->thread = std::thread(data_->run_function, sup_type, sup_info, agg_type, agg_info);
}

}
}
