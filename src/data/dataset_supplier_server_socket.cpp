#include "data/dataset_supplier_server_socket.h"

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

class DatasetSupplierServerSocket::Private {
public:
  
  void RegisterCallback() {
    server->SetRegistrationCallback(
      std::bind(&Private::OnClientRegistered, this, std::placeholders::_1, std::placeholders::_2)
    );
  }
  
  void OnClientRegistered(std::shared_ptr<tcp::socket> pull_socket, uint32_t client_id);
  
  int id;
  std::string buffer;
    
  std::unique_ptr<DatasetSocketServer> server;
};

void DatasetSupplierServerSocket::Private::OnClientRegistered(std::shared_ptr<tcp::socket> push_socket, uint32_t client_id) {
  Log::Print(LogLevel::Info, "[Sup %d] Client trying to register", id);
  
  // Send dataset -1 to tell client there is no more dataset
  uint32_t dataset_id = -1;
  push_socket->send(asio::buffer(&dataset_id, 4));
  
}

DatasetSupplierServerSocket::DatasetSupplierServerSocket() : data_(new Private) {
  data_->id = GetId();
}

DatasetSupplierServerSocket::~DatasetSupplierServerSocket() {
  Close();
}

void DatasetSupplierServerSocket::Bind(const std::string & info) {

  data_->server.reset(new DatasetSocketServer);
  data_->server->Init();
  data_->server->StartRegistration(info);
  
  Log::Print(LogLevel::Info, "[Sup %d] Server started", GetId());
}

void DatasetSupplierServerSocket::Close() {
  // Register callback for client trying to connect during registration stop
  data_->RegisterCallback();
  data_->server->StopRegistration();
  data_->server->Close();
}

std::string DatasetSupplierServerSocket::GetInfo() const {
  return data_->server->GetInfo();
}

void DatasetSupplierServerSocket::PushDataset(ConstDatasetPtr dataset, uint32_t dataset_id) {
  
  uint32_t length = dataset->GetSize();
  data_->buffer.resize(length);
  dataset->RetrieveBuffer(data_->buffer);
  
  tcp::socket* push_socket = nullptr;
  size_t client_id = -1;
  
  SocketMap & client_sockets = data_->server->GetClientSockets();
  
  // It is probable that there is no client enregistered yet
  // so wait for at least one
  while(client_sockets.empty()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  
  std::vector<size_t> client_ids;
  for(auto & entry : client_sockets) {
    client_ids.push_back(entry.first);
  }
  
  client_id = client_ids[dataset_id%client_ids.size()];
  push_socket = client_sockets[client_id].get();
  
  Log::Print(LogLevel::Debug, "[Sup %d][socket] Pushing dataset %d to client %d...", GetId(), dataset_id, client_id);
  
  push_socket->send(asio::buffer(&dataset_id, 4));
  push_socket->send(asio::buffer(&length, 4));
  push_socket->send(asio::buffer(data_->buffer));
  
  Log::Print(LogLevel::Debug, "[Sup %d][socket] Pushed dataset %d to client %d", GetId(), dataset_id, client_id);
  
}

void DatasetSupplierServerSocket::PushEndSignal() {
  
  uint32_t dataset_id = -1;
  
  // At this level, registration is too late, stop it
  
  // Register callback for client trying to connect during registration stop
  data_->server->SetRegistrationCallback(
    std::bind(&Private::OnClientRegistered, data_.get(), std::placeholders::_1, std::placeholders::_2)
  );
  data_->server->StopRegistration();
  
  // If any client has registered, it shall be taken into account
  SocketMap & client_sockets = data_->server->GetClientSockets();
  
  // Tell clients to stop receiving by sending them end signal
  // close their socket right after that
  for(auto & entry : client_sockets) {
    uint32_t client_id = entry.first;
    tcp::socket* client_socket = entry.second.get();
    client_socket->send(asio::buffer(&dataset_id, 4));
    
    Log::Print(LogLevel::Info, "[Sup %d][socket] Client %d unregistered", GetId(), client_id);
  }
  
  Log::Print(LogLevel::Info, "[Sup %d][socket] All clients unregistered", GetId());
  
}
  
}
}
