#include <memory>
#include <fstream>
#include <vector>
#include <thread>
#include <cstring>

#include <log/log.h>

#include <data/data.h>
#include <data/data_exception.h>
#include <data/dataset.h>
#include <data/dataset_supplier_client_socket.h>
#include <data/dataset_aggregator_client_socket.h>
#include <data/dataset_supplier_client_file.h>
#include <data/dataset_aggregator_client_file.h>

using cwa::Log;
using cwa::LogLevel;

using cwa::data::Dataset;
using cwa::data::DatasetPtr;
using cwa::data::ConstDatasetPtr;
using cwa::data::Data;
using cwa::data::DataPtr;

using cwa::data::CannotConnectToServerException;

using cwa::data::DatasetSupplierClientPtr;
using cwa::data::DatasetAggregatorClientPtr;
using cwa::data::DatasetSupplierClientSocket;
using cwa::data::DatasetAggregatorClientSocket;
using cwa::data::DatasetSupplierClientFile;
using cwa::data::DatasetAggregatorClientFile;

int main(int argc, char* argv[]) {
    
  try {
    
    Log::Init(argc, argv);
    Log::Print(LogLevel::Info, "[Client app] Application started.");
    
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
      Log::Print(LogLevel::Info, "[Client app] Using asio");
    } else if (sup_agg_type == "--file") {
      Log::Print(LogLevel::Info, "[Client app] Using file");
    } else {
      cwa::Log::Print(cwa::LogLevel::Fatal, "[Client app] Unknown agg. sup. comm");
      exit(-1);
    }
    
    std::string supplier_info, aggregator_info;
    
    if (sup_agg_type == "--asio") {
      supplier_info = "127.0.0.1:15550";
      aggregator_info = "127.0.0.1:15553";
    } else if(sup_agg_type == "--file") {
      supplier_info = "mp_ring_supplier.json";
      aggregator_info = "mp_ring_aggregator.json";
    }
        
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
      
      dataset_aggregator->Connect(supplier_info);
    } catch (CannotConnectToServerException &) {
      Log::Print(LogLevel::Info, "[Worker] Server is not reachable, may be shutdown");
      exit(0);
    }
    
    try {
      if (sup_agg_type == "--asio") {
        dataset_supplier.reset(new DatasetSupplierClientSocket());
      } else if(sup_agg_type == "--file") {
        dataset_supplier.reset(new DatasetSupplierClientFile());
      }
      
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
    
    dataset_supplier->PushEndSignal();
    
    Log::Print(LogLevel::Info, "[Worker][Sup %d] Finished with %d datasets", dataset_supplier->GetId(), datasets);
    
  } catch(std::exception& e) {
    
    Log::Print(LogLevel::Error, "[Client app] Exception: %s", e.what());
    return -1;
    
  }
  
  return 0;
    
}
