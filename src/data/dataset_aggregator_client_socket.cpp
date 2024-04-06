#include "data/dataset_aggregator_client_socket.h"

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

class DatasetAggregatorClientSocket::Private {
public:
  int id;
  std::string buffer;    
  std::unique_ptr<DatasetSocketClient> client;
};

DatasetAggregatorClientSocket::DatasetAggregatorClientSocket() : data_(new Private) {
  data_->id = GetId();
}

DatasetAggregatorClientSocket::~DatasetAggregatorClientSocket() {
  Close();
}

void DatasetAggregatorClientSocket::Connect(const std::string & info) {
  data_->client.reset(new DatasetSocketClient);
  data_->client->Register(info, GetTimeout());
  
  Log::Print(LogLevel::Info, "[Agg %d] Registered as client %d", GetId(),
    data_->client->GetId());
}

void DatasetAggregatorClientSocket::Close() {
  
}

void DatasetAggregatorClientSocket::PullDataset(DatasetPtr& dataset, uint32_t & dataset_id) {
  uint32_t length = -1;
  
  tcp::socket* pull_socket = nullptr;
  uint32_t client_id = -1;

  pull_socket = &data_->client->GetPushPullSocket();
  
  Log::Print(LogLevel::Debug, "[Agg %d][socket] Pulling dataset from server...", GetId());
  
  try {
    pull_socket->receive(asio::buffer(&dataset_id, 4));
  } catch (std::exception& e) {
    Log::Print(LogLevel::Error, "[Agg %d][socket] Exception: %s", GetId(), e.what());
    exit(-1);
  }
  
  if(dataset_id == -1) {
    return;
  }
  
  Log::Print(LogLevel::Debug, "[Agg %d][socket] Pulling dataset %d from server...", GetId(), dataset_id);
  
  pull_socket->receive(asio::buffer(&length, 4));
  data_->buffer.resize(length);
  
  // One call to receive won't retrieve all data
  // Each call returns bytes retrieved, sum them
  // to get the whole data
  size_t received_bytes = 0;
  while(received_bytes < length) {
    char* buffer = &data_->buffer[received_bytes];
    size_t bytes = pull_socket->receive(asio::buffer(buffer, length - received_bytes));
    
    received_bytes += bytes;
  }
  
  Log::Print(LogLevel::Debug, "[Agg %d][socket] Pulled dataset %d from server", GetId(), dataset_id);
  
  // In case dataset has not been allocated yet, do it now
  if(!dataset) dataset.reset(new Dataset);

  dataset->SetId("dataset_" + std::to_string(dataset_id));
  dataset->RetainBuffer(data_->buffer);
}

void DatasetAggregatorClientSocket::HandleEndSignal() {
  Log::Print(LogLevel::Info, "[Agg %d][socket] Unregistered from server as client %d",
    GetId(), data_->client->GetId());
}
  
}
}
