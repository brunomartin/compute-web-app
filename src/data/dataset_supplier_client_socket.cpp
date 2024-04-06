#include "data/dataset_supplier_client_socket.h"

#include <map>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <asio.hpp>

#include <log/log.h>

#include "data/utils.h"
#include "data/data_exception.h"
#include "data/dataset_socket_server.h"
#include "data/dataset_socket_client.h"

using asio::ip::tcp;

using cwa::Log;
using cwa::LogLevel;

using std::chrono::high_resolution_clock;

namespace cwa {
namespace data {

class DatasetSupplierClientSocket::Private {
public:
  
  int id;
  std::string buffer;
    
  std::unique_ptr<DatasetSocketServer> server;
  std::unique_ptr<DatasetSocketClient> client;
};

DatasetSupplierClientSocket::DatasetSupplierClientSocket() : data_(new Private) {
  data_->id = GetId();
}

DatasetSupplierClientSocket::~DatasetSupplierClientSocket() {
  
}

void DatasetSupplierClientSocket::Connect(const std::string & info) {
  data_->client.reset(new DatasetSocketClient);
  data_->client->Register(info, GetTimeout());
  
  Log::Print(LogLevel::Info, "[Sup %d] Registered as client %d", GetId(),
    data_->client->GetId());
}

void DatasetSupplierClientSocket::Close() {
  
}

void DatasetSupplierClientSocket::PushDataset(std::shared_ptr<const Dataset> dataset, uint32_t dataset_id) {
  
  uint32_t length = dataset->GetSize();
  data_->buffer.resize(length);
  dataset->RetrieveBuffer(data_->buffer);
  
  tcp::socket* push_socket = nullptr;
  size_t client_id = -1;

  push_socket = &data_->client->GetPushPullSocket();
  
  Log::Print(LogLevel::Debug, "[Sup %d][socket] Pushing dataset %d to server...", GetId(), dataset_id);
  
  push_socket->send(asio::buffer(&dataset_id, 4));
  push_socket->send(asio::buffer(&length, 4));
  push_socket->send(asio::buffer(data_->buffer));
  
  Log::Print(LogLevel::Debug, "[Sup %d][socket] Pushed dataset %d to server", GetId(), dataset_id);
}

void DatasetSupplierClientSocket::PushEndSignal() {
  
  uint32_t dataset_id = -1;
  
  tcp::socket & push_socket = data_->client->GetPushPullSocket();
  push_socket.send(asio::buffer(&dataset_id, 4));
  push_socket.close();
  
  Log::Print(LogLevel::Info, "[Sup %d][socket] Unregistered from server as client %d",
    GetId(), data_->client->GetId());
  
}
  
}
}
