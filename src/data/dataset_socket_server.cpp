#include "data/dataset_socket_server.h"

#include <map>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <regex>

#include <asio.hpp>

#include <log/log.h>

#include "data/utils.h"
#include "data/data.h"
#include "data/data_exception.h"
#include "data/dataset.h"


using cwa::Log;
using cwa::LogLevel;

using std::chrono::high_resolution_clock;

#define USE_ASYNC_ACCEPT

namespace cwa {
namespace data {

uint32_t DatasetSocketServer::next_client_id = 1;

DatasetSocketServer::DatasetSocketServer() {
  
}

DatasetSocketServer::~DatasetSocketServer() {
  StopRegistration();
  Close();
}

void DatasetSocketServer::Init() {  
  io_service_thread_ = std::thread([&](){ io_service_.run(); });
}

void DatasetSocketServer::Close() {
  io_service_.stop();
  if(io_service_thread_.joinable()) io_service_thread_.join();
  acceptor_.reset();
}

void DatasetSocketServer::StartRegistration(const std::string & url) {
  
  // Check if matches 127.0.0.1:4123, 127.0.0.1:*, *:1233 or *:*
  std::regex regex("(\\*|\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}):(\\*|\\d{1,5})");
  if (!std::regex_match(url, regex)) {
    throw Exception();
  }
  
  std::string raw_ip_address = Split<':'>(url)[0];
  uint16_t port = std::atoi(Split<':'>(url)[1].c_str());
  
  tcp::endpoint endpoint;
  try {
    asio::ip::address ip_address = asio::ip::address::from_string(raw_ip_address);
    endpoint = tcp::endpoint(ip_address, port);
  } catch(std::runtime_error & e) {
    endpoint = tcp::endpoint(tcp::v4(), port);
  }
  
  acceptor_.reset(new tcp::acceptor(io_service_, endpoint));
  
  // Set linger to 0 so that acceptor does not take any new connection after
  // being destroyed
  acceptor_->set_option(asio::socket_base::linger(false, 0));
  
  DoAccept_();
}

void DatasetSocketServer::StopRegistration() {
  stop_registering_ = true;
}

void DatasetSocketServer::DoAccept_() {
  acceptor_->async_accept([this](const asio::error_code &error, tcp::socket register_socket) {

    Log::Print(LogLevel::Info, "[asio server] New accepted connection");
    
    uint32_t client_id = next_client_id;
    register_socket.send(asio::buffer(&client_id, 4));
    
    tcp::acceptor pull_acceptor(io_service_, tcp::endpoint(tcp::v4(), 0));
    tcp::endpoint push_endpoint = pull_acceptor.local_endpoint();
    uint32_t push_pull_port = push_endpoint.port();
        
    register_socket.send(asio::buffer(&push_pull_port, 4));
    
    Log::Print(LogLevel::Info, "[asio server] Opening push/pull socket on port %d", push_pull_port);
    tcp::socket client_socket = tcp::socket(io_context_);
    
    pull_acceptor.accept(client_socket);
    
    Log::Print(LogLevel::Info, "[asio server] Client %d push/pull socket connected", client_id);
    
    // Close register socket as it won't be used anymore
    register_socket.close();    
    
    // This socket will be used to send/receive serveral messages
    // so add keep alive option to this socket
    client_socket.set_option(asio::socket_base::keep_alive(true));
    
    auto client_socket_ptr = std::make_shared<tcp::socket>(std::move(client_socket));
    
    // Add current socket to client socket only if still registering
    if(!stop_registering_) {
      client_sockets_[client_id] = client_socket_ptr;
    }
    
    if(on_client_registered_) on_client_registered_(client_socket_ptr, client_id);
    
    next_client_id++;
        
    DoAccept_();
  });
}

std::string DatasetSocketServer::GetInfo() const {
  tcp::endpoint endpoint = acceptor_->local_endpoint();
  std::string result = endpoint.address().to_string();
  result += ":" + std::to_string(endpoint.port());
  return result;
}

SocketMap & DatasetSocketServer::GetClientSockets() {
  return client_sockets_;
}

void DatasetSocketServer::SetRegistrationCallback(Callback on_client_registered) {
  on_client_registered_ = on_client_registered;
}
  
}
}
