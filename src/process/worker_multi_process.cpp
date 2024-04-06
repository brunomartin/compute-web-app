#include "worker_multi_process.h"

#include <thread>
#include <cstdlib>
#include <vector>
#include <string>
#include <sstream>

#include <Poco/Process.h>
#include <Poco/Exception.h>

#include <log/log.h>

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

using cwa::Log;
using cwa::LogLevel;

using cwa::data::Data;
using cwa::data::DataPtr;
using cwa::data::Dataset;

using cwa::data::DatasetAggregatorClientPtr;
using cwa::data::DatasetSupplierClientPtr;

using cwa::data::DatasetAggregatorServerFile;
using cwa::data::DatasetSupplierServerFile;
using cwa::data::DatasetAggregatorServerSocket;
using cwa::data::DatasetSupplierServerSocket;

namespace cwa {
namespace process {

class WorkerMultiProcess::Private  {
public:
  
  std::shared_ptr<Poco::ProcessHandle> process;
  
};

WorkerMultiProcess::WorkerMultiProcess(const Definition & definition, const VariantMap & parameter_values,
  DatasetSupplierServerPtr supplier, DatasetAggregatorServerPtr aggregator) :
  Worker(definition, parameter_values, supplier, aggregator), data_(new Private) {
      
}

WorkerMultiProcess::~WorkerMultiProcess() {
  
}


void WorkerMultiProcess::Launch() {
  
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
    
  std::string command = GetDefinition().GetCommand();
  auto args = GetDefinition().GetArgs();
  
  std::vector<std::string> paramater_args;
  
  for(const auto & entry : GetParameterValues()) {
    paramater_args.push_back(entry.first);
    std::stringstream ss;
    ss << entry.second;
    paramater_args.push_back(ss.str());
  }
  
  args.insert(args.end(), paramater_args.begin(), paramater_args.end());
  
  std::vector<std::string> cwa_args = {
    "--cwa-sup-type", sup_type,
    "--cwa-sup-info", sup_info,
    "--cwa-agg-type", agg_type,
    "--cwa-agg-info", agg_info,
  };
  
  args.insert(args.end(), cwa_args.begin(), cwa_args.end());
  
  int rc;
  try {
    Poco::ProcessHandle process = Poco::Process::launch(command, args);
    rc = process.id();
    data_->process = std::make_shared<Poco::ProcessHandle>(process);
  } catch (Poco::SystemException& e) {
    Log::Print(LogLevel::Error, "Exception: %s", e.what());
  }
  
}

}
}
