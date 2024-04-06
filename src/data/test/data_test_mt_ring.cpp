#include <memory>
#include <fstream>
#include <vector>
#include <thread>
#include <cstring>

#include <log/log.h>

#include <data/data.h>
#include <data/data_exception.h>
#include <data/dataset.h>
#include <data/data_supply_state.h>
#include <data/data_supplier.h>
#include <data/data_aggregator.h>
#include <data/dataset_supplier_server_socket.h>
#include <data/dataset_supplier_client_socket.h>
#include <data/dataset_aggregator_server_socket.h>
#include <data/dataset_aggregator_client_socket.h>
#include <data/dataset_supplier_server_file.h>
#include <data/dataset_supplier_client_file.h>
#include <data/dataset_aggregator_server_file.h>
#include <data/dataset_aggregator_client_file.h>

using cwa::Log;
using cwa::LogLevel;

using cwa::data::Data;
using cwa::data::DataPtr;
using cwa::data::ConstDataPtr;
using cwa::data::Dataset;
using cwa::data::DatasetPtr;
using cwa::data::ConstDatasetPtr;

using cwa::data::CannotConnectToServerException;

using cwa::data::DataSupplyState;
using cwa::data::DataSupplyStatePtr;
using cwa::data::DataSupplier;
using cwa::data::DataAggregator;

using cwa::data::DatasetSupplierServerPtr;
using cwa::data::DatasetAggregatorServerPtr;
using cwa::data::DatasetSupplierServerSocket;
using cwa::data::DatasetAggregatorServerSocket;
using cwa::data::DatasetSupplierServerFile;
using cwa::data::DatasetAggregatorServerFile;

using cwa::data::DatasetSupplierClientPtr;
using cwa::data::DatasetAggregatorClientPtr;
using cwa::data::DatasetSupplierClientSocket;
using cwa::data::DatasetAggregatorClientSocket;
using cwa::data::DatasetSupplierClientFile;
using cwa::data::DatasetAggregatorClientFile;

enum class ExpectedResult {
  None,
  ServerTimeout,
  NoMoreDataset
};

void StartWorkers(const std::string & sup_agg_type, int workers, int timeous_ms, ExpectedResult expected_result);
void RunWorker(const std::string & sup_agg_type, const std::string & supplier_info, const std::string & aggregator_info, int timeous_ms, ExpectedResult expected_result);

int main(int argc, char* argv[]) {
  
  Log::Init(argc, argv);
  Log::Print(LogLevel::Info, "Application started");
  
  std::string sup_agg_type = "--asio";
  
  for(int i=1;i<argc;i++) {
    if(strcmp(argv[i], "--asio") == 0) {
      sup_agg_type = "--asio";
    }
    
    if(strcmp(argv[i], "--file") == 0) {
      sup_agg_type = "--file";
    }
  }
  
  if (sup_agg_type == "--asio") {
    Log::Print(LogLevel::Info, "Using asio");
  } else if (sup_agg_type == "--file") {
    Log::Print(LogLevel::Info, "Using file");
  } else {
    cwa::Log::Print(cwa::LogLevel::Fatal, "Unknown agg. sup. comm");
    exit(-1);
  }
  
  std::string input_dir = "mt_ring_input";
  std::string output_dir = "mt_ring_output";
  
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
    file.write((const char*) data_values.data(),
               data_values.size()*sizeof(uint16_t));
    file.close();
  }
  
  Log::Print(LogLevel::Info, "Files written.");
  
  std::string supplier_info, aggregator_info;
  
  if (sup_agg_type == "--asio") {
    supplier_info = "127.0.0.1:14550";
    aggregator_info = "127.0.0.1:14553";
  } else if(sup_agg_type == "--file") {
    supplier_info = "mt_ring_supplier.json";
    aggregator_info = "mt_ring_aggregator.json";
  }
  
  // Remove exchange files if any
  if (sup_agg_type == "--file") {
    std::remove((supplier_info).c_str());
    std::remove((supplier_info + ".lock").c_str());
    std::remove((aggregator_info).c_str());
    std::remove((aggregator_info + ".lock").c_str());
  }
  
  // Start worker before starting servers, shall timeout
  std::thread workers_thread_0(&StartWorkers, sup_agg_type, 2, 100, ExpectedResult::ServerTimeout);
  
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  
  DataPtr input_data(new Data(input_dir));
  for(int i=0;i<datasets;i++) {
    DatasetPtr dataset(new Dataset);
    dataset->SetId("dataset_" + std::to_string(i));
    input_data->AddDataset(dataset);
  }
  
  DataPtr output_data(new Data(output_dir));
    
  DataSupplyStatePtr supply_state(new DataSupplyState(input_data));
  
  DatasetSupplierServerPtr dataset_supplier;
  DatasetAggregatorServerPtr dataset_aggregator;
  
  // Start worker with higher timeout, shall all get work to do
  std::thread workers_thread_1(&StartWorkers, sup_agg_type, workers, 2000, ExpectedResult::None);
  
  if(sup_agg_type == "--asio") {
    dataset_supplier.reset(new DatasetSupplierServerSocket());
    dataset_aggregator.reset(new DatasetAggregatorServerSocket());
  } else if(sup_agg_type == "--file") {
    dataset_supplier.reset(new DatasetSupplierServerFile());
    dataset_aggregator.reset(new DatasetAggregatorServerFile());
  }
  
  dataset_supplier->Bind(supplier_info);
  dataset_aggregator->Bind(aggregator_info);
    
  DataSupplier data_supplier(dataset_supplier);
  DataAggregator data_aggregator(dataset_aggregator);
  
  if(sup_agg_type == "--asio") {
    data_aggregator.SetDatasetPulledCallback([](DatasetPtr dataset) {
      dataset->Persist();
      dataset->ReleaseBuffer();
    });
  }
    
  Log::Print(LogLevel::Info, "Starting supplier and aggregator...");
  data_aggregator.Start(output_data);
  data_supplier.Start(supply_state);
  
  Log::Print(LogLevel::Info, "Waiting for data aggregation...");
  while(output_data->GetDatasets().size() < input_data->GetDatasets().size()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  Log::Print(LogLevel::Info, "Data aggregated.");
  
  // While supplier does not send dataset index -1, aggregator is running.
  // When it receives it, aggregator leaves its loop and thread is finished.
  Log::Print(LogLevel::Info, "Waiting for supplier and aggregator to finish...");
  data_supplier.Wait();
  data_aggregator.Wait();
  
  // Wait for worker that has done a timeout
  workers_thread_0.join();
  
  // Wait for workers that effectively worked
  workers_thread_1.join();
  
  Log::Print(LogLevel::Info, "Supplier and aggregator finished.");
  
  Log::Print(LogLevel::Info, "Checking new workers registered does not get any data...");
  // Start worker after all is done, shall return without doing nothing
  std::thread workers_thread_2(&StartWorkers, sup_agg_type, 2, 100, ExpectedResult::NoMoreDataset);
  workers_thread_2.join();
  
  Log::Print(LogLevel::Info, "CLosing dataset supplier and aggregator...");
  dataset_supplier->Close();
  dataset_aggregator->Close();
  
  Log::Print(LogLevel::Info, "Checking new workers cannot register anymore...");
  // Start worker after closing servers, shall timeout
  std::thread workers_thread_3(&StartWorkers, sup_agg_type, 2, 100, ExpectedResult::ServerTimeout);
  workers_thread_3.join();
  
  Log::Print(LogLevel::Info, "Checking aggregated data...");
  
  // Check output and input data have same number of datasets
  if(input_data->GetDatasets().size() != output_data->GetDatasets().size()) {
    Log::Print(LogLevel::Error, "Supplier and aggregator data have not the"
               " same number of datasets.");
    Log::Print(LogLevel::Info, "TEST FAILED");
    return -1;
  }
  
  // Check output and input data are consistent
  std::string input_buffer, output_buffer;
  for(size_t i=0;i<input_data->GetDatasets().size();i++) {
    ConstDatasetPtr input_dataset = input_data->GetDatasets()[i];
    input_dataset->RetrieveBuffer(input_buffer);
    
    // Output dataset may not arrive in same order, so for each input dataset,
    // we have to find the corresponding one in output datasets
    ConstDatasetPtr output_dataset;
    for(size_t j=0;j<output_data->GetDatasets().size();j++) {
      output_dataset = output_data->GetDatasets()[j];
      if(output_dataset->GetId() == input_dataset->GetId()) break;
    }
    
    output_dataset->RetrieveBuffer(output_buffer);
    
    if(input_buffer != output_buffer) {
      Log::Print(LogLevel::Error, "Output dataset different for index %d", i);
      Log::Print(LogLevel::Info, "TEST FAILED");
      return -1;
    }
  }
  
  Log::Print(LogLevel::Info, "TEST PASSED");
  
  return 0;
}


void StartWorkers(const std::string & sup_agg_type, int workers, int timeous_ms, ExpectedResult expected_result) {
  
  std::vector<std::thread> worker_threads;
  
  std::string supplier_info, aggregator_info;
  
  if(sup_agg_type == "--asio") {
    supplier_info = "127.0.0.1:14550";
    aggregator_info = "127.0.0.1:14553";
  } else if(sup_agg_type == "--file") {
    supplier_info = "mt_ring_supplier.json";
    aggregator_info = "mt_ring_aggregator.json";
  }

  for(int i=0;i<workers;i++) {
    
    std::this_thread::sleep_for(std::chrono::milliseconds(i*20));
    
    worker_threads.push_back(
      std::thread(&RunWorker, sup_agg_type, supplier_info, aggregator_info, timeous_ms, expected_result)
    );
  }
  
  for(auto & worker_thread : worker_threads) {
    worker_thread.join();
  }
  
}

void RunWorker(const std::string & sup_agg_type, const std::string & supplier_info, const std::string & aggregator_info, int timeous_ms, ExpectedResult expected_result) {
    
  DatasetAggregatorClientPtr dataset_aggregator;
  DatasetSupplierClientPtr dataset_supplier;
  
  std::string output_dir;
  DataPtr output_data;
  
  if(sup_agg_type == "--file") {
    output_dir = "mt_ring_output";
    output_data.reset((new Data(output_dir)));
  }
  
  try {
    if (sup_agg_type == "--asio") {
      dataset_aggregator.reset(new DatasetAggregatorClientSocket());
    } else if(sup_agg_type == "--file") {
      dataset_aggregator.reset(new DatasetAggregatorClientFile());
    }
    
    dataset_aggregator->SetTimeout(timeous_ms);
    dataset_aggregator->Connect(supplier_info);
  } catch (CannotConnectToServerException &) {
    Log::Print(LogLevel::Info, "[Worker] Server is not reachable, may be shutdown");
    if(expected_result == ExpectedResult::ServerTimeout) {
      return;
    } else {
      Log::Print(LogLevel::Error, "[Worker] No server timeout when expeted");
      Log::Print(LogLevel::Info, "TEST FAILED");
      exit(-1);
    }
  }
  
  try {
    if (sup_agg_type == "--asio") {
      dataset_supplier.reset(new DatasetSupplierClientSocket());
    } else if(sup_agg_type == "--file") {
      dataset_supplier.reset(new DatasetSupplierClientFile());
    }
    
    dataset_supplier->SetTimeout(timeous_ms);
    dataset_supplier->Connect(aggregator_info);
  } catch (CannotConnectToServerException &) {
    Log::Print(LogLevel::Info, "[Worker] Server is not reachable anymore");
    exit(-1);
  }

  DatasetPtr dataset(new Dataset);
  uint32_t index;
  int datasets = 0;
  
  while(true) {
    Log::Print(LogLevel::Info, "[Worker][Agg %d] Pulling dataset...", dataset_aggregator->GetId());
    dataset_aggregator->PullDataset(dataset, index);
    Log::Print(LogLevel::Info, "[Worker][Agg %d] Pulled dataset %d", dataset_aggregator->GetId(), index);
    
    if(index == -1) {
      dataset_aggregator->HandleEndSignal();
      dataset_supplier->PushEndSignal();
      break;
    }
    
    if (sup_agg_type == "--file") {
      output_data->AddDataset(dataset);
      dataset->Persist();
      dataset->ReleaseBuffer();
    }
    
    datasets++;
    
    Log::Print(LogLevel::Info, "[Worker][Sup %d] Pushing dataset %d...", dataset_supplier->GetId(), index);
    dataset_supplier->PushDataset(dataset, index);
    Log::Print(LogLevel::Info, "[Worker][Sup %d] Pushed dataset %d", dataset_supplier->GetId(), index);
  }
  
  if(expected_result == ExpectedResult::NoMoreDataset) {
    if(datasets != 0) {
      Log::Print(LogLevel::Error, "[Worker] Still dataset when no more is expeted");
      Log::Print(LogLevel::Info, "TEST FAILED");
      exit(-1);
    }
  }
  
  Log::Print(LogLevel::Info, "[Worker][Sup %d] Finished with %d datasets", dataset_supplier->GetId(), datasets);
}
