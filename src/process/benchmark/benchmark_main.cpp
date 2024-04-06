#include <memory>
#include <fstream>
#include <vector>
#include <thread>
#include <cstring>
#include <iostream>
#include <random>
#include <algorithm>

#include <log/log.h>

#include <data/data.h>
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
  
  std::string sup_type = "socket";
  std::string agg_type = "socket";
  size_t worker_count = 2;
  
  // If generate files, the program do only this
  bool generate_files = false;
  size_t datasets = 20;
  size_t dataset_height = 512;
  
  int algorithm = 0;
    
  for(int i=1;i<argc;i++) {
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
    
    if(strcmp(argv[i], "--workers") == 0) {
      if(i+1 < argc) {
        worker_count = atoi(argv[i+1]);
      }
    }
    
    if(strcmp(argv[i], "--generate") == 0) {
      generate_files = true;
    }
    
    if(strcmp(argv[i], "--datasets") == 0) {
      if(i+1 < argc) {
        datasets = atoi(argv[i+1]);
      }
    }
    
    if(strcmp(argv[i], "--dataset-height") == 0) {
      if(i+1 < argc) {
        dataset_height = atoi(argv[i+1]);
      }
    }
    
    if(strcmp(argv[i], "--algorithm") == 0) {
      if(i+1 < argc) {
        algorithm = atoi(argv[i+1]);
      }
    }
  }
  
  std::string input_dir = "input";
  std::string output_dir = "output";
  
  if(generate_files) {
    // Generate data
    std::vector<uint16_t> data_values;
    data_values.resize(dataset_height*dataset_height);
    
    std::default_random_engine dre;
    std::uniform_int_distribution<int> di(0, std::pow(2, 15));
    
    // Write test files
    Log::Print(LogLevel::Info, "Writing files...");
    for(int i=0;i<datasets;i++) {
      std::generate(data_values.begin(), data_values.end(), [&]{ return di(dre);});
      std::ofstream file(input_dir + "/dataset_" + std::to_string(i) + ".raw");
      file.write((const char*) data_values.data(), data_values.size()*sizeof(uint16_t));
      file.close();
    }
    Log::Print(LogLevel::Info, "Files written.");
    return 0;
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
  
  auto start = std::chrono::high_resolution_clock::now();
  
  DataPtr input_data(new Data(input_dir));
  for(int i=0;i<datasets;i++) {
    DatasetPtr dataset(new Dataset);
    dataset->SetId("dataset_" + std::to_string(i));
    input_data->AddDataset(dataset);
  }
  
  DataPtr output_data(new Data(output_dir));
  
  // This is a benchmark, no parameter are needed
  std::string algorithm_str = std::to_string(algorithm);
  Definition definition("./CwaProcessBenchmarkWorker", {"-q", "--algorithm", algorithm_str});
  
  // If running worker on remote machine using ssh
//  definition = Definition("ssh", {"pi@192.168.1.40", "./CwaProcessBenchmarkWorker",
//    "-q", "--algorithm", std::to_string(algorithm)
//  });
  
  VariantMap parameter_values;
  
  DatasetSupplierServerPtr dataset_supplier;
  DatasetAggregatorServerPtr dataset_aggregator;
  
  std::string sup_info, agg_info;
  
  if(sup_type == "socket") {
    dataset_supplier.reset(new DatasetSupplierServerSocket());
    sup_info = "*:*";
  } else if(sup_type == "file") {
    dataset_supplier.reset(new DatasetSupplierServerFile());
    sup_info = "supplier.json";
  }
  
  if(agg_type == "socket") {
    dataset_aggregator.reset(new DatasetAggregatorServerSocket());
    agg_info = "*:*";
  } else if(agg_type == "file") {
    dataset_aggregator.reset(new DatasetAggregatorServerFile());
    agg_info = "aggregator.json";
  }
  
  DataSupplier data_supplier(dataset_supplier);
  DataAggregator data_aggregator(dataset_aggregator);
  
  DataSupplyStatePtr supply_state(new DataSupplyState(input_data));
  
  // Start data supplier and aggregtor
  dataset_supplier->Bind(sup_info);
  dataset_aggregator->Bind(agg_info);
  
  // If worker count is less or equal to 0, give informations
  // for manual started worker to interact
  if(worker_count <= 0) {
    std::cout << "supplier type: " << sup_type << std::endl;
    std::cout << "supplier info: " << dataset_supplier->GetInfo() << std::endl;
    std::cout << "aggregator type: " << agg_type << std::endl;
    std::cout << "aggregator type: " << dataset_aggregator->GetInfo() << std::endl;
  } else {
    std::vector<WorkerPtr> workers;
    for(int i=0;i<worker_count;i++) {
      WorkerPtr worker;
      worker.reset(new WorkerMultiProcess(definition, parameter_values, dataset_supplier, dataset_aggregator));
      workers.push_back(worker);
    }
    
    // Start process via workers launch
    for(WorkerPtr worker : workers) {
      worker->Launch();
    }
  }
  
  Log::Print(LogLevel::Info, "Starting data process...");
  
  data_supplier.Start(supply_state);
  data_aggregator.Start(output_data);
  
  Log::Print(LogLevel::Info, "Waiting for processed data...");
  while(output_data->GetDatasets().size() < input_data->GetDatasets().size()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  Log::Print(LogLevel::Info, "Data processed.");
  
  auto end = std::chrono::high_resolution_clock::now();
  
  auto duration = std::chrono::duration<double, std::milli>(end-start);
  
  std::cout << duration.count() << std::endl;
  
  Log::Print(LogLevel::Info, "Waiting for supplier and aggregator...");
  data_supplier.Wait();
  data_aggregator.Wait();
  
  Log::Print(LogLevel::Info, "Supplier and aggregator finished.");
    
  Log::Print(LogLevel::Info, "Data process finished");
  
  return 0;
}
