#ifndef CWA_DATA_DATASET_SOCKET_CLIENT_H
#define CWA_DATA_DATASET_SOCKET_CLIENT_H

#include <asio.hpp>

using asio::ip::tcp;

namespace cwa {
namespace data {

class DatasetSocketClient {
public:
  DatasetSocketClient();
  ~DatasetSocketClient();
  
  void Register(const std::string & url, int timeout_ms);
  
  uint32_t GetId() const;
  tcp::socket & GetPushPullSocket();
  
private:
  asio::io_context io_context_;
  tcp::socket push_pull_socket_ = tcp::socket(io_context_);
  
  uint32_t client_id_ = -1;
  
};

}
}

#endif // CWA_DATA_DATASET_SOCKET_CLIENT_H
