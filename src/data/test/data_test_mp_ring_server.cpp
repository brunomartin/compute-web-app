#include <memory>
#include <fstream>
#include <vector>
#include <thread>
#include <cstring>

#include <log/log.h>

#include <data/data.h>
#include <data/dataset.h>
#include <data/data_supply_state.h>
#include <data/data_supplier.h>
#include <data/data_aggregator.h>
#include <data/dataset_supplier_server_socket.h>
#include <data/dataset_aggregator_server_socket.h>
#include <data/dataset_supplier_server_file.h>
#include <data/dataset_aggregator_server_file.h>

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

using cwa::data::DatasetSupplierServerPtr;
using cwa::data::DatasetAggregatorServerPtr;
using cwa::data::DatasetSupplierServerSocket;
using cwa::data::DatasetAggregatorServerSocket;
using cwa::data::DatasetSupplierServerFile;
using cwa::data::DatasetAggregatorServerFile;

int main(int argc, char* argv[]) {
  
  try {
    
    Log::Init(argc, argv);
    Log::Print(LogLevel::Info, "Multi process server application started.");
      
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
      Log::Print(LogLevel::Info, "[Server app] Using asio");
    } else if (sup_agg_type == "--file") {
      Log::Print(LogLevel::Info, "[Server app] Using file");
    } else {
      cwa::Log::Print(cwa::LogLevel::Fatal, "[Server app] Unknown agg. sup. comm");
      exit(-1);
    }
      
    size_t datasets = 20;
    std::vector<uint16_t> data_values;
    data_values.resize(2*1024*1024);
    
    // Write test files
    Log::Print(LogLevel::Info, "Writing files...");
    for(int i=0;i<datasets;i++) {
      for(int j=0;j<data_values.size();j++) {
        data_values[j] = i + j;
      }
      
      std::ofstream file("mp_ring_input/dataset_" + std::to_string(i) + ".raw");
      file.write((const char*) data_values.data(),
                 data_values.size()*sizeof(uint16_t));
      file.close();
    }
    
    Log::Print(LogLevel::Info, "Files written.");
    
    std::string supplier_info, aggregator_info;
    
    if (sup_agg_type == "--asio") {
      supplier_info = "127.0.0.1:15550";
      aggregator_info = "127.0.0.1:15553";
    } else if(sup_agg_type == "--file") {
      supplier_info = "mp_ring_supplier.json";
      aggregator_info = "mp_ring_aggregator.json";
    }
    
    // Remove exchange files if any
    if (sup_agg_type == "--file") {
      std::remove((supplier_info).c_str());
      std::remove((supplier_info + ".lock").c_str());
      std::remove((aggregator_info).c_str());
      std::remove((aggregator_info + ".lock").c_str());
    }
    
    DataPtr input_data(new Data("mp_ring_input"));
    for(int i=0;i<datasets;i++) {
      DatasetPtr dataset(new Dataset);
      dataset->SetId("dataset_" + std::to_string(i));
      input_data->AddDataset(dataset);
    }
    
    DataSupplyStatePtr supply_state(new DataSupplyState(input_data));
    
    DatasetSupplierServerPtr dataset_supplier;
    DatasetAggregatorServerPtr dataset_aggregator;
    
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
    
    DataPtr output_data(new Data("mp_ring_output"));
    
    Log::Print(LogLevel::Info, "Starting supplier and aggregator...");
    data_supplier.Start(supply_state);
    data_aggregator.Start(output_data);
    
    // While supplier does not send dataset index -1, aggregator is running.
    // When it receives it, aggregator leaves its loop and thread is finished.
    Log::Print(LogLevel::Info, "Waiting for supplier and aggregator to finish...");
    data_supplier.Wait();
    data_aggregator.Wait();
    
    Log::Print(LogLevel::Info, "Waiting for data aggregation...");
    while(output_data->GetDatasets().size() != input_data->GetDatasets().size()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    Log::Print(LogLevel::Info, "Data aggregated.");
    
    Log::Print(LogLevel::Info, "Supplier and aggregator finished.");
    
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
    
  } catch(std::exception& e) {
    
    Log::Print(LogLevel::Error, "[Server app] Exception: %s", e.what());
    return -1;
    
  }
  
  return 0;
}
