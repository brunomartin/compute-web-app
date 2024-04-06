#include "data/dataset_socket_client.h"

#include <string>
#include <chrono>
#include <csignal>
#include <regex>

#include <asio.hpp>

#include <log/log.h>

#include "data/utils.h"
#include "data/data_exception.h"


using cwa::Log;
using cwa::LogLevel;

using std::chrono::high_resolution_clock;
using std::chrono::milliseconds;

namespace cwa {
namespace data {

DatasetSocketClient::DatasetSocketClient() {
  // Ignore SIGPIPE signal that may be emited from socket.connect()
  // if server is not available
  signal(SIGPIPE, SIG_IGN);
}

DatasetSocketClient::~DatasetSocketClient() {
  
}

void DatasetSocketClient::Register(const std::string & url, int timeout_ms) {
  
  // Check if matches 127.0.0.1:4123, 127.0.0.1:*, *:1233 or *:*
  std::regex regex("(\\*|\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}):(\\*|\\d{1,5})");
  if (!std::regex_match(url, regex)) {
    throw Exception();
  }
  
  std::string address = Split<':'>(url)[0];
  uint16_t port = std::atoi(Split<':'>(url)[1].c_str());
  tcp::endpoint endpoint(asio::ip::address::from_string(address), port);
      
  tcp::socket register_socket = tcp::socket(io_context_);
    
  // Try to connect to server for an amount of time before giving up
  auto next = high_resolution_clock::now() + milliseconds(timeout_ms);
  
  while(high_resolution_clock::now() < next) {
    
    try {
      register_socket.connect(endpoint);
      break;
    } catch(std::runtime_error & e) {
      register_socket.close();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }
  }
  
  if(!register_socket.is_open()) {
    Log::Print(LogLevel::Error, "[asio client] Could not connect to %s after %dms",
      url.c_str(), timeout_ms);
    throw CannotConnectToServerException();
  }
  
  register_socket.receive(asio::buffer(&client_id_, 4));
  
  Log::Print(LogLevel::Info, "[asio client %d] Connected to %s", client_id_, url.c_str());
      
  uint32_t pull_port = -1;
  register_socket.receive(asio::buffer(&pull_port, 4));
  
  tcp::endpoint push_pull_endpoint(endpoint.address(), pull_port);
  push_pull_socket_.connect(push_pull_endpoint);
  
  // This socket will be used to send/receive serveral messages
  // so add keep alive option to this socket
  push_pull_socket_.set_option(asio::socket_base::keep_alive(true));
    
  Log::Print(LogLevel::Info, "[asio client %d] Push/pull socket connected", client_id_);
  
  // Close register socket as it won't be used anymore
  register_socket.close();
}

uint32_t DatasetSocketClient::GetId() const {
  return client_id_;
}

tcp::socket & DatasetSocketClient::GetPushPullSocket() {
  return push_pull_socket_;
}
  
}
}
