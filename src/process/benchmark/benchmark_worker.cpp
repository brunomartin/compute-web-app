#include <string>
#include <cstring>
#include <iostream>

#include <Eigen/Dense>

#include <log/log.h>

#include <data/data.h>
#include <data/dataset_supplier_client_file.h>
#include <data/dataset_aggregator_client_file.h>
#include <data/dataset_supplier_client_socket.h>
#include <data/dataset_aggregator_client_socket.h>

using cwa::Log;
using cwa::LogLevel;

using cwa::data::Data;
using cwa::data::DataPtr;
using cwa::data::Dataset;
using cwa::data::DatasetPtr;
using cwa::data::ConstDatasetPtr;

using cwa::data::DatasetAggregatorClientPtr;
using cwa::data::DatasetSupplierClientPtr;

using cwa::data::DatasetAggregatorClientFile;
using cwa::data::DatasetSupplierClientFile;
using cwa::data::DatasetAggregatorClientSocket;
using cwa::data::DatasetSupplierClientSocket;

typedef Eigen::Matrix<uint16_t, Eigen::Dynamic, Eigen::Dynamic> UInt16Matrix;

int main(int argc, char* argv[]) {
  
  Log::Init(argc, argv);
  Log::Print(LogLevel::Info, "Worker started");
  
  std::string sup_type = "socket";
  std::string agg_type = "socket";
  
  std::string sup_info = "";
  std::string agg_info = "";
  
  int algorithm = 0;
  
  for(int i=1;i<argc;i++) {
    if(strcmp(argv[i], "--cwa-sup-type") == 0) {
      if(i+1 < argc) {
        sup_type = argv[i+1];
      }
    }
  
    if(strcmp(argv[i], "--cwa-sup-info") == 0) {
      if(i+1 < argc) {
        sup_info = argv[i+1];
      }
    }
  
    if(strcmp(argv[i], "--cwa-agg-type") == 0) {
      if(i+1 < argc) {
        agg_type = argv[i+1];
      }
    }
    
    if(strcmp(argv[i], "--cwa-agg-info") == 0) {
      if(i+1 < argc) {
        agg_info = argv[i+1];
      }
    }
    
    if(strcmp(argv[i], "--algorithm") == 0) {
      if(i+1 < argc) {
        algorithm = atoi(argv[i+1]);
      }
    }
  }

  DatasetAggregatorClientPtr aggregator;
  DatasetSupplierClientPtr supplier;
  
  std::string output_dir;
  DataPtr output_data;
  
  DatasetPtr dataset_in, dataset_out;
  ConstDatasetPtr const_dataset_in;
  uint32_t index;
  
  if(sup_type == "file") {
    aggregator.reset(new DatasetAggregatorClientFile());
    Log::Print(LogLevel::Info, "Worker using file aggregator, %s", sup_info.c_str());
  } else if(sup_type == "socket") {
    aggregator.reset(new DatasetAggregatorClientSocket());
    Log::Print(LogLevel::Info, "Worker using socket aggregator, %s", sup_info.c_str());
  } else {
    cwa::Log::Print(cwa::LogLevel::Fatal, "Unknown supplier type: %s", sup_type.c_str());
    exit(-1);
  }
  
  if(agg_type == "file") {
    supplier.reset(new DatasetSupplierClientFile());
    Log::Print(LogLevel::Info, "Worker using file supplier, %s", agg_info.c_str());
  } else if(agg_type == "socket") {
    supplier.reset(new DatasetSupplierClientSocket());
    Log::Print(LogLevel::Info, "Worker using socket supplier, %s", agg_info.c_str());
  } else {
    cwa::Log::Print(cwa::LogLevel::Fatal, "Unknown aggregator type: %s", agg_type.c_str());
    exit(-1);
  }
  
  if(agg_type == "file") {
    output_dir = "output";
    output_data.reset((new Data(output_dir)));
  }
  
  aggregator->Connect(sup_info);
  supplier->Connect(agg_info);
  
  dataset_out.reset(new Dataset);
  
  UInt16Matrix matrix;
  std::string buffer_out;
    
  while(true) {
    aggregator->PullDataset(dataset_in, index);
    
    if(index == -1) {
      aggregator->HandleEndSignal();
      break;
    }
    
    dataset_out->SetId(dataset_in->GetId());
    
    const std::string & buffer = dataset_in->GetBuffer();
    
    int dataset_height = std::sqrt(buffer.size()/2);
    
    if(matrix.size() == 0) {
      matrix = UInt16Matrix::Random(dataset_height, dataset_height);
      buffer_out.resize(buffer.size());
    }

    Eigen::Map<const UInt16Matrix> dataset_matrix((const uint16_t*)buffer.data(), dataset_height, dataset_height);
    Eigen::Map<UInt16Matrix> result((uint16_t*)buffer_out.data(), dataset_height, dataset_height);
  
    size_t rows = dataset_height;
    size_t cols = dataset_height;
    
    // Applying product on the entire matrix will consume all
    // CPU caches. No effect of parallization will be seen
    switch(algorithm) {
      case 0:
        result.setZero();
        for(size_t row = 0;row<rows;row++) {
          for(size_t col = 0;col<cols;col++) {
            result(row, col) = 0;
            for(size_t index = 0;index<cols;index++) {
              result(row, col) += matrix(row, index) * dataset_matrix(index, col);
            }
          }
        }
        break;
      case 1:
        result = matrix * dataset_matrix;
        break;
      default:
        exit(-1);
    }

    dataset_out->RetainBuffer(buffer_out);
    
    if(agg_type == "file") {
      output_data->AddDataset(dataset_out);
      dataset_out->Persist();
      dataset_out->ReleaseBuffer();
    }
    
    supplier->PushDataset(dataset_out, index);
  }
  
  supplier->PushEndSignal();
  
  Log::Print(LogLevel::Info, "Worker finished");
  
  return 0;
}
