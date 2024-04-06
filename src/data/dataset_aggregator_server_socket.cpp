#include "data/dataset_aggregator_server_socket.h"

#include <map>
#include <string>
#include <queue>
#include <mutex>

#include <asio.hpp>

#include <log/log.h>

#include "data/dataset_socket_server.h"
#include "data/dataset_socket_client.h"
#include "data/utils.h"
#include "data/data.h"
#include "data/data_exception.h"
#include "data/dataset.h"

using asio::ip::tcp;

using cwa::Log;
using cwa::LogLevel;

using std::chrono::high_resolution_clock;

namespace cwa {
namespace data {

class DatasetAggregatorServerSocket::Private {
public:
    
  struct DatasetStruct {
    uint32_t client_id;
    uint32_t id;
    std::string buffer;
  };
  
  std::queue<DatasetStruct> datasets;
  std::mutex datasets_mutex;
  
  void RegisterCallback() {
    server->SetRegistrationCallback(
      std::bind(&Private::OnClientRegistered, this, std::placeholders::_1, std::placeholders::_2)
    );
  }
  
  void OnClientRegistered(std::shared_ptr<tcp::socket> pull_socket, uint32_t client_id);
  
  int id;
  std::string buffer;
    
  std::unique_ptr<DatasetSocketServer> server;
  
  std::vector<std::thread> client_threads;
};

void DatasetAggregatorServerSocket::Private::OnClientRegistered(std::shared_ptr<tcp::socket> pull_socket, uint32_t client_id) {
  Log::Print(LogLevel::Info, "[Agg %d] Client registered", id);
  
  std::thread thread([this, pull_socket, client_id]() {
    
    uint32_t length = -1;
    
    DatasetStruct dataset;
    dataset.client_id = client_id;
    
    while(true) {
      pull_socket->receive(asio::buffer(&dataset.id, 4));
      
      if(dataset.id == -1) {
        std::lock_guard<std::mutex> lock(datasets_mutex);
        datasets.push(std::move(dataset));
        break;
      }
      
      pull_socket->receive(asio::buffer(&length, 4));
      dataset.buffer.resize(length);
      
      // One call to receive won't retrieve all data
      // Each call returns bytes retrieved, sum them
      // to get the whole data
      size_t received_bytes = 0;
      while(received_bytes < length) {
        char* buffer_ptr = &dataset.buffer[received_bytes];
        size_t bytes = pull_socket->receive(asio::buffer(buffer_ptr, length - received_bytes));
        
        received_bytes += bytes;
      }
      
      Log::Print(LogLevel::Info, "[Agg %d] Dataset %d received from client %d", id, dataset.id, dataset.client_id);
      
      std::lock_guard<std::mutex> lock(datasets_mutex);
      datasets.push(std::move(dataset));
    }
    
  });
  
  client_threads.push_back(std::move(thread));
}

DatasetAggregatorServerSocket::DatasetAggregatorServerSocket() : data_(new Private) {
  data_->id = GetId();
}

DatasetAggregatorServerSocket::~DatasetAggregatorServerSocket() {
  Close();
}

void DatasetAggregatorServerSocket::Bind(const std::string & info) {
  data_->server.reset(new DatasetSocketServer);
  
  data_->RegisterCallback();
  
  data_->server->Init();
  data_->server->StartRegistration(info);
  
  Log::Print(LogLevel::Info, "[Agg %d] Server started", GetId());
}

void DatasetAggregatorServerSocket::Close() {
  for(std::thread & thread : data_->client_threads) {
    if(thread.joinable()) thread.join();
  }
  data_->server->StopRegistration();
  data_->server->Close();
}

std::string DatasetAggregatorServerSocket::GetInfo() const {
  return data_->server->GetInfo();
}

void DatasetAggregatorServerSocket::PullDataset(DatasetPtr& dataset, uint32_t & dataset_id) {
  
  uint32_t length = -1;
  
  tcp::socket* pull_socket = nullptr;
  uint32_t client_id = -1;
  
  // A queue is filled with arrived datasets
  // Wait until at least one dataset is in the queue
  while(data_->datasets.empty()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  
  std::unique_lock<std::mutex> lock(data_->datasets_mutex);
  Private::DatasetStruct dataset_str = std::move(data_->datasets.front());
  data_->datasets.pop();
  lock.unlock();
  
  client_id = dataset_str.client_id;
  dataset_id = dataset_str.id;

  Log::Print(LogLevel::Debug, "[Agg %d][socket] Pulling dataset %d from client %d...", GetId(), dataset_id, client_id);

  data_->buffer = std::move(dataset_str.buffer);
  
  Log::Print(LogLevel::Debug, "[Agg %d][socket] Pulled dataset %d from server", GetId(), dataset_id);
  
  if(dataset_id == -1) {
    SocketMap & client_sockets = data_->server->GetClientSockets();
          
    if(client_id == -1) {
      Log::Print(LogLevel::Fatal, "[Agg %d][socket] Client does not exist", GetId());
    }
    
    Log::Print(LogLevel::Info, "[Agg %d][socket] Client %d removed", GetId(), client_id);
    client_sockets.erase(client_id);
    return;
  }
  
  // In case dataset has not been allocated yet, do it now
  if(!dataset) dataset.reset(new Dataset);

  dataset->SetId("dataset_" + std::to_string(dataset_id));
  dataset->RetainBuffer(data_->buffer);
}

void DatasetAggregatorServerSocket::HandleEndSignal() {
  SocketMap & client_sockets = data_->server->GetClientSockets();
  
  if(client_sockets.empty()) {
    // At this level, registration is too late, stop it
    data_->server->StopRegistration();
    
    Log::Print(LogLevel::Info, "[Agg %d][socket] No more supplier", GetId());
    throw NoMoreSupplierException();
  }
}
  
}
}
