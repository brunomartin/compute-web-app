#include <memory>
#include <fstream>
#include <vector>
#include <thread>
#include <cstring>

#include <log/log.h>

#include <data/data.h>
#include <data/utils.h>
#include <data/data_supply_state.h>
#include <data/data_supplier.h>
#include <data/data_aggregator.h>
#include <data/data_exception.h>

#include <data/dataset_supplier_server_file.h>
#include <data/dataset_aggregator_server_file.h>
#include <data/dataset_supplier_server_socket.h>
#include <data/dataset_aggregator_server_socket.h>

#include <process/variant.h>
#include <process/parameter.h>
#include <process/definition.h>
#include <process/worker_multi_thread.h>
#include <process/worker_multi_process.h>

#include "test_worker.h"

using cwa::Log;
using cwa::LogLevel;

using cwa::data::Data;
using cwa::data::DataPtr;
using cwa::data::ConstDataPtr;
using cwa::data::Dataset;
using cwa::data::DatasetPtr;
using cwa::data::ConstDatasetPtr;

using cwa::data::DataSupplyState;
using cwa::data::DataSupplyStatePtr;
using cwa::data::DataSupplier;
using cwa::data::DataAggregator;

using cwa::data::DatasetAggregatorServerFile;
using cwa::data::DatasetSupplierServerFile;
using cwa::data::DatasetAggregatorServerSocket;
using cwa::data::DatasetSupplierServerSocket;

using cwa::process::Parameter;
using cwa::process::ParameterMap;
using cwa::process::Variant;
using cwa::process::Variants;
using cwa::process::VariantMap;
using cwa::process::Definition;

using cwa::process::Worker;
using cwa::process::WorkerPtr;
using cwa::process::WorkerMultiThread;
using cwa::process::WorkerMultiProcess;

int main(int argc, char* argv[]) {
  
  Log::Init(argc, argv);
  Log::Print(LogLevel::Info, "Application started");
  
  std::string worker_type = "mt";
  std::string sup_type = "file";
  std::string agg_type = "file";
  
  for(int i=1;i<argc;i++) {
    if(strcmp(argv[i], "--cwa-worker") == 0) {
      if(i+1 < argc) {
        worker_type = argv[i+1];
      }
    }
    
    if(strcmp(argv[i], "--cwa-sup") == 0) {
      if(i+1 < argc) {
        sup_type = argv[i+1];
      }
    }
  
    if(strcmp(argv[i], "--cwa-agg") == 0) {
      if(i+1 < argc) {
        agg_type = argv[i+1];
      }
    }
  }
    
  if (worker_type == "mt") {
    Log::Print(LogLevel::Info, "Using multi thread worker");
  } else if (worker_type == "mp") {
    Log::Print(LogLevel::Info, "Using multi process worker");
  } else {
    cwa::Log::Print(cwa::LogLevel::Fatal, "Unknown worker type: %s", worker_type.c_str());
    exit(-1);
  }
  
  if (sup_type == "socket") {
    Log::Print(LogLevel::Info, "Using socket supplier");
  } else if (sup_type == "file") {
    Log::Print(LogLevel::Info, "Using file supplier");
  } else {
    cwa::Log::Print(cwa::LogLevel::Fatal, "Unknown supplier type: %s", sup_type.c_str());
    exit(-1);
  }
  
  if (agg_type == "socket") {
    Log::Print(LogLevel::Info, "Using socket aggregator");
  } else if (agg_type == "file") {
    Log::Print(LogLevel::Info, "Using file aggregator");
  } else {
    cwa::Log::Print(cwa::LogLevel::Fatal, "Unknown aggregator type: %s", sup_type.c_str());
    exit(-1);
  }
  
  std::string input_dir = "process_mt_input";
  std::string output_dir = "process_mt_output";
  
  size_t datasets = 20;
  size_t workers = 6;
  std::vector<uint16_t> data_values;
  data_values.resize(2*1024*1024);
  
  // Write test files
  Log::Print(LogLevel::Info, "Writing files...");
  for(int i=0;i<datasets;i++) {
    for(int j=0;j<data_values.size();j++) {
      data_values[j] = i + j;
    }
    
    std::ofstream file(input_dir + "/dataset_" + std::to_string(i) + ".raw");
    file.write((const char*) data_values.data(), data_values.size()*sizeof(uint16_t));
    file.close();
  }
  
  Log::Print(LogLevel::Info, "Files written.");
  
  DataPtr input_data(new Data(input_dir));
  for(int i=0;i<datasets;i++) {
    DatasetPtr dataset(new Dataset);
    dataset->SetId("dataset_" + std::to_string(i));
    input_data->AddDataset(dataset);
  }
  
  DataPtr output_data(new Data(output_dir));
  
  Definition definition("./CwaProcessTestWorkerApp");
  
  definition.AddParameter(Parameter({
    .name = "test-integer",
    .description = "test integer",
    .default_value = 5
  }));
  
  definition.AddParameter(Parameter({
    .name = "test-decimal",
    .description = "test decimal",
    .default_value = 43.5
  }));
  
  definition.AddParameter(Parameter({
    .name = "test-string",
    .description = "test string",
    .default_value = "--file"
  }));
  
  definition.AddParameter(Parameter({
    .name = "test-vector",
    .description = "test vector",
    .default_value = Variants({42, 1e-6, "test"})
  }));
  
  VariantMap parameter_values = definition.GetDefaultParameterValues();
  
  parameter_values["test-integer"] = 10;
  parameter_values["test-decimal"] = 18.3;
  parameter_values["test-string"] = "test";
  
  std::string args_str = Variant::SerializeValues(parameter_values);
  
  auto args = Split<' '>(args_str);
  
  int worker_argc = args.size() + 1;
  char* worker_argv[worker_argc];
  
  for(int i=0;i<worker_argc-1;i++) {
    worker_argv[i+1] = (char*)args[i].data();
  }
  
  VariantMap parsed_parameter_values = TestWorker::ParseArguments(worker_argc, worker_argv);
  
  if(parameter_values != parsed_parameter_values) {
    Log::Print(LogLevel::Fatal, "Serialize/Parse error");
    Log::Print(LogLevel::Fatal, "TEST FAILED");
    return -1;
  }
  
  DatasetSupplierServerPtr dataset_supplier;
  DatasetAggregatorServerPtr dataset_aggregator;
  
  std::string sup_info, agg_info;
  
  if(sup_type == "socket") {
    dataset_supplier.reset(new DatasetSupplierServerSocket());
    sup_info = "127.0.0.1:*";
  } else if(sup_type == "file") {
    dataset_supplier.reset(new DatasetSupplierServerFile());
    sup_info = "process_mt_supplier.json";
  }
  
  if(agg_type == "socket") {
    dataset_aggregator.reset(new DatasetAggregatorServerSocket());
    agg_info = "127.0.0.1:*";
  } else if(agg_type == "file") {
    dataset_aggregator.reset(new DatasetAggregatorServerFile());
    agg_info = "process_mt_aggregator.json";
  }
  
  DataSupplier data_supplier(dataset_supplier);
  DataAggregator data_aggregator(dataset_aggregator);
  
  DataSupplyStatePtr supply_state(new DataSupplyState(input_data));
  
  TestWorker test_worker(parameter_values);
  
  std::vector<WorkerPtr> mt_workers;  
  for(int i=0;i<workers;i++) {
    WorkerPtr worker;
    if(worker_type == "mt") {
      
      WorkerMultiThread::RunFunction test_worker_run = std::bind(&TestWorker::RunFunction, &test_worker,
                                                                 std::placeholders::_1, std::placeholders::_2,
                                                                 std::placeholders::_3, std::placeholders::_4);
      
      worker.reset(new WorkerMultiThread(definition, parameter_values, dataset_supplier,
                                         dataset_aggregator, test_worker_run));
    } else if(worker_type == "mp") {
      worker.reset(new WorkerMultiProcess(definition, parameter_values, dataset_supplier,
                                         dataset_aggregator));
    }    
    mt_workers.push_back(worker);
  }
      
  Log::Print(LogLevel::Info, "Starting data process...");
  
  // Start data supplier and aggregtor
  dataset_supplier->Bind(sup_info);
  dataset_aggregator->Bind(agg_info);
  
  data_supplier.Start(supply_state);
  data_aggregator.Start(output_data);
  
  // Start process via workers launch
  for(WorkerPtr & worker : mt_workers) {
    worker->Launch();
  }
  
  Log::Print(LogLevel::Info, "Waiting for processed data...");
  while(output_data->GetDatasets().size() < input_data->GetDatasets().size()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  Log::Print(LogLevel::Info, "Data processed.");
  
  Log::Print(LogLevel::Info, "Waiting for supplier and aggregator...");
  data_supplier.Wait();
  data_aggregator.Wait();
  
  Log::Print(LogLevel::Info, "Supplier and aggregator finished.");
  
  Log::Print(LogLevel::Info, "Checking processed data...");
  
  // Check output and input data have same number of datasets
  if(input_data->GetDatasets().size() != output_data->GetDatasets().size()) {
    Log::Print(LogLevel::Error, "Output data has not expected number of datasets.");
    Log::Print(LogLevel::Info, "TEST FAILED");
    return -1;
  }
  
  // Check output and input data are consistent
  std::string input_buffer, output_buffer, expected_output_buffer;
  for(size_t i=0;i<input_data->GetDatasets().size();i++) {
    ConstDatasetPtr const_input_dataset = input_data->GetDatasets()[i];
    input_data->GetDatasets()[i]->RetrieveBuffer(input_buffer);
    
    DatasetPtr input_dataset(new Dataset);
    input_dataset->SetId(const_input_dataset->GetId());
    input_dataset->RetainBuffer(input_buffer);
    
    // Output dataset may not arrive in same order, so for each input dataset,
    // we have to find the corresponding one in output datasets
    ConstDatasetPtr output_dataset;
    for(size_t j=0;j<output_data->GetDatasets().size();j++) {
      output_dataset = output_data->GetDatasets()[j];
      if(output_dataset->GetId() == input_dataset->GetId()) break;
    }
    
    output_dataset->RetrieveBuffer(output_buffer);
    
    const_input_dataset = std::const_pointer_cast<const Dataset>(input_dataset);
    DatasetPtr expected_output_dataset;
    
    test_worker.ComputeDataset(const_input_dataset, expected_output_dataset);
    
    expected_output_buffer = expected_output_dataset->GetBuffer();
        
    if(output_buffer != expected_output_buffer) {
      Log::Print(LogLevel::Error, "Output dataset different for index %d", i);
      Log::Print(LogLevel::Info, "TEST FAILED");
      return -1;
    }
  }
  
  Log::Print(LogLevel::Info, "TEST PASSED");
  
  return 0;
}
