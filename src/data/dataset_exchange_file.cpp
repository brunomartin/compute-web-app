#include "data/dataset_exchange_file.h"

#include <map>
#include <string>
#include <thread>
#include <fstream>
#include <chrono>
#include <set>

#include <fcntl.h>
#include <unistd.h>
#include <csignal>

#include <nlohmann/json.hpp>

#include <log/log.h>

#include "data/utils.h"
#include "data/data_exception.h"

using cwa::Log;
using cwa::LogLevel;

using std::chrono::high_resolution_clock;
using std::chrono::milliseconds;

namespace cwa {
namespace data {

void ReadJson(const std::string & filename, nlohmann::json & json) {
  try {
    std::ifstream file(filename, std::ios::in);
    file >> json;
  } catch (std::exception& e) {
    json = nlohmann::json();
  }
}

void WriteJson(const std::string & filename, const nlohmann::json & json) {
  std::ofstream file(filename, std::ios::trunc);
  file << json;
}

DatasetExchangeFile::DatasetExchangeFile(const std::string & filename) : filename_(filename) {
  // Ignore SIGPIPE signal that may be emited from file open when
  // trying to check existence of filename_. As it is handled in
  // the code, we can safely ignore this signal.
  signal(SIGPIPE, SIG_IGN);
}

DatasetExchangeFile::~DatasetExchangeFile() {
  
}

const std::string & DatasetExchangeFile::GetFilename() const {
  return filename_;
}

void DatasetExchangeFile::WaitForFile(int timeout_ms) const {
  
  bool file_exists = false;
  auto next = high_resolution_clock::now() + milliseconds(timeout_ms);
  while(high_resolution_clock::now() < next) {
    std::ifstream file(filename_);
    
    if(file.is_open()) {
      file_exists = true;
      break;
    }
  }
  
  if(!file_exists) {
    throw TimeoutException();
  }
}

void DatasetExchangeFile::RemoveFiles() {
  std::remove((filename_).c_str());
  std::remove((filename_ + ".lock").c_str());
}

void DatasetExchangeFile::LockFile() {
  
  std::string lock_filename = filename_ + ".lock";
  bool locked = false;
    
  int timeout_ms = 5000;
  auto next = high_resolution_clock::now() + milliseconds(timeout_ms);
  
  // Try to create a lock file
  while(high_resolution_clock::now() < next) {
    int lock_file_fd = open(lock_filename.c_str(), O_WRONLY | O_CREAT | O_EXCL);
    
    if(lock_file_fd != -1) {
      locked = true;
      // Close lock file, it will be removed by this process when required
      close(lock_file_fd);
      break;
    }
    std::this_thread::sleep_for(milliseconds(10));
  }  
      
  if(!locked) {
    Log::Print(LogLevel::Error, "[file] File has not been locked");
  }
}

void DatasetExchangeFile::UnlockFile() {
  // Remove lock file
  std::remove((filename_ + ".lock").c_str());
}


void DatasetExchangeFile::Init(bool is_aggregator) {
  nlohmann::json json;
  
  json["datasets"] = {};
  
  if(is_aggregator)  {
    // supplier clients shall send end of dataset signals
    // aggregator server has to know the id of clients to
    // safely unregister them, clients property is mandatory
    // for safely unregister clients from this server
    json["clients"] = {};
    json["unregistered_clients"] = {};
  }
  
  WriteJson(filename_, json);
}

uint32_t DatasetExchangeFile::RegisterClient() {
  nlohmann::json json;
  ReadJson(filename_, json);
  
  uint32_t client_id = json["clients"].size() + 1;
  json["clients"].push_back(client_id);
  
  WriteJson(filename_, json);
  
  return client_id;
}

void DatasetExchangeFile::PushDataset(const Dataset & dataset) {
  
  nlohmann::json dataset_json;
  dataset_json["id"] = dataset.id;
  dataset_json["filename"] = dataset.filename;
  if(dataset.client_id != -1) {
    dataset_json["client_id"] = dataset.client_id;
  }
  
  nlohmann::json json;
  ReadJson(filename_, json);
  json["datasets"].push_back(dataset_json);
  WriteJson(filename_, json);
}

bool DatasetExchangeFile::HasDataset() const {
  nlohmann::json json;
  ReadJson(filename_, json);
  return !json["datasets"].empty();
}

DatasetExchangeFile::Dataset DatasetExchangeFile::GetFirstDataset() {
  nlohmann::json json;
  ReadJson(filename_, json);
  
  Dataset dataset;
  dataset.id = json["datasets"][0]["id"];
  dataset.filename = json["datasets"][0]["filename"];
  if(json["datasets"][0].contains("client_id")) {
    dataset.client_id = json["datasets"][0]["client_id"];
  }
  
  return dataset;
}

void DatasetExchangeFile::PopFirstDataset() {
  nlohmann::json json;
  ReadJson(filename_, json);
  json["datasets"].erase(0);
  WriteJson(filename_, json);
}

void DatasetExchangeFile::UnregisterClient(uint32_t client_id) {
  nlohmann::json json;
  ReadJson(filename_, json);
  
  // Remove it from the registered clients
  std::set<uint32_t> client_ids;
  if(!json["unregistered_clients"].is_null()) {
    json["unregistered_clients"].get_to(client_ids);
  }
  client_ids.insert(client_id);
  json["unregistered_clients"] = client_ids;
  
  WriteJson(filename_, json);
}

bool DatasetExchangeFile::AreAllClientsUnregistered() const {
  nlohmann::json json;
  ReadJson(filename_, json);
  
  return json["clients"] == json["unregistered_clients"];
}
  
}
}
