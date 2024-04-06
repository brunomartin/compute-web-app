#include <string>
#include <cstring>

#include <log/log.h>

#include <data/utils.h>

#include "test_worker.h"

using cwa::Log;
using cwa::LogLevel;

using cwa::process::Variant;

int main(int argc, char* argv[]) {
  
  Log::Init(argc, argv);
  Log::Print(LogLevel::Info, "Worker started");
  
  std::string sup_type = "socket";
  std::string agg_type = "socket";
  
  std::string sup_info = "";
  std::string agg_info = "";
  
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
  }
  
  VariantMap parameter_values = TestWorker::ParseArguments(argc, argv);
    
  TestWorker worker(parameter_values);
  
  worker.RunFunction(sup_type, sup_info, agg_type, agg_info);
  
  Log::Print(LogLevel::Info, "Worker finished");
  
  return 0;
}
