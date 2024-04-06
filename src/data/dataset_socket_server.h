#ifndef CWA_DATA_DATASET_SOCKET_SERVER_H
#define CWA_DATA_DATASET_SOCKET_SERVER_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <map>
#include <asio.hpp>

using asio::ip::tcp;

namespace cwa {
namespace data {

typedef std::map<uint32_t, std::shared_ptr<tcp::socket>> SocketMap;

class DatasetSocketServer {
public:
  DatasetSocketServer();
  ~DatasetSocketServer();
  
  typedef std::function<void(std::shared_ptr<tcp::socket>,uint32_t)> Callback;
  void SetRegistrationCallback(Callback on_client_registered);
  
  void Init();
  void Close();
  void StartRegistration(const std::string & url);
  void StopRegistration();
  
  std::string GetInfo() const;
  SocketMap & GetClientSockets();
  
private:
  asio::io_context io_context_;
  
  asio::io_service io_service_;
  asio::io_service::work work_ = asio::io_service::work(io_service_);
  std::thread io_service_thread_;
  
  std::thread register_thread_;
  bool stop_registering_ = false;
  std::mutex register_socket_mutex_;
  std::condition_variable register_socket_cv_;
  std::string register_end_point_;
  
  tcp::socket server_socket_ = tcp::socket(io_context_);
  static uint32_t next_client_id;
  std::map<uint32_t, std::shared_ptr<tcp::socket>> client_sockets_;
  
  void RunRegistration_(uint16_t port);
  
  void DoAccept_();
  std::unique_ptr<tcp::acceptor> acceptor_;
  
  Callback on_client_registered_ = nullptr;
};

}
}

#endif // CWA_DATA_DATASET_SOCKET_SERVER_H
