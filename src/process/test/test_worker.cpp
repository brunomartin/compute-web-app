#include "test_worker.h"

#include <sstream>
#include <cstring>

#include <log/log.h>

#include <data/data.h>
#include <data/utils.h>
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

using cwa::data::DatasetAggregatorClientFile;
using cwa::data::DatasetSupplierClientFile;
using cwa::data::DatasetAggregatorClientSocket;
using cwa::data::DatasetSupplierClientSocket;

using cwa::process::Variant;
using cwa::process::Variants;

class TestWorker::Private {
public:
  int test_integer;
  double test_decimal;
  std::string test_string;
  Variants test_vector;
  
};

TestWorker::TestWorker(const VariantMap & parameters) : data_(new Private) {
  data_->test_integer = parameters.at("test-integer").ToInteger();
  data_->test_decimal = parameters.at("test-decimal").ToDecimal();
  data_->test_string = parameters.at("test-string").ToString();
  data_->test_vector = parameters.at("test-vector").ToVariants();
}

VariantMap TestWorker::ParseArguments(int argc, char* argv[]) {
  
  VariantMap result;
  for(int i=1;i<argc;i++) {
    if(strcmp(argv[i], "test-integer") == 0) {
      if(i+1 < argc) {
        result["test-integer"] = std::atoi(argv[i+1]);
      }
    }

    if(strcmp(argv[i], "test-decimal") == 0) {
      if(i+1 < argc) {
        result["test-decimal"] = std::atof(argv[i+1]);
      }
    }

    if(strcmp(argv[i], "test-string") == 0) {
      if(i+1 < argc) {
        result["test-string"] = argv[i+1];
      }
    }

    if(strcmp(argv[i], "test-vector") == 0) {
      if(i+1 < argc) {
        std::vector<std::string> args = Split<','>(argv[i+1]);
        
        if(args.size() == 3) {
          Variants vector;
          vector.push_back(std::atoi(args[0].c_str()));
          vector.push_back(std::atof(args[1].c_str()));
          vector.push_back(args[2]);
          result["test-vector"] = vector;
        }

      }
    }
  }
  
  return result;
}

void TestWorker::RunFunction(const std::string& sup_type, const std::string& sup_info, const std::string& agg_type, const std::string& agg_info) {
  
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
    output_dir = "process_mt_output";
    output_data.reset((new Data(output_dir)));
  }
  
  aggregator->Connect(sup_info);
  supplier->Connect(agg_info);
    
  while(true) {
    aggregator->PullDataset(dataset_in, index);
    
    if(index == -1) {
      aggregator->HandleEndSignal();
      break;
    }
    
    const_dataset_in = std::const_pointer_cast<const Dataset>(dataset_in);
    ComputeDataset(const_dataset_in, dataset_out);
    
    if(agg_type == "file") {
      output_data->AddDataset(dataset_out);
      dataset_out->Persist();
      dataset_out->ReleaseBuffer();
    }
    
    supplier->PushDataset(dataset_out, index);
  }
  
  supplier->PushEndSignal();  
}

void TestWorker::ComputeDataset(ConstDatasetPtr& input, DatasetPtr& output) {
  output.reset(new Dataset);
    
  output->SetId(input->GetId());
  
  std::string buffer = input->GetBuffer();
  uint16_t* value = (uint16_t*)buffer.data();
  
  // Do a process dependent on parameter
  for(size_t i=0;i<2*1024*1024;i++) {
    value[i] *= 2;
    value[i] -= data_->test_integer;
    value[i] *= data_->test_decimal;
  }
  
  output->RetainBuffer(buffer);
}
