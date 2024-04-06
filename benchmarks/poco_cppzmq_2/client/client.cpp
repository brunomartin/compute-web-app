#include <zmq.hpp>
#include <iostream>
#include <chrono>
#include <fstream>
#include <regex>

#include "process.h"

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::microseconds;

bool log_trace = false;
bool log_info = false;

int main(int argc, char* argv[])
{
  std::string server_host = "tcp://127.0.0.1";
  uint16_t server_port = 5555;
  
  // Uncomment in order to test ipc for communication
//  server_host = "ipc:///tmp/cwa_server";
  
  // First argument is server address
  if(argc > 1) {
    server_host = argv[1];
  }
  
  std::string server_address;
  if(server_host.substr(0, 3) == "tcp") {
    server_address = server_host + ":" + std::to_string(server_port);
  } else if (server_host.substr(0, 3) == "ipc") {
    server_address = server_host;    
  } else {
    std::cerr << "Bad server address" << std::endl;
    exit(-1);
  }
  
  std::string server_end_point = server_address;
  
  zmq::context_t ctx;
  zmq::socket_t sock_server(ctx, zmq::socket_type::req);
  
  zmq::message_t msg;
  
  if(log_info) std::cout << "[WORKER] Connect to server " << server_end_point << std::endl;
  sock_server.connect(server_end_point);
    
  // Send worker address to server
  sock_server.send(zmq::message_t(std::string(server_host)), zmq::send_flags::none);

  // server reply contains worker id as a uint32
  if(log_info) std::cout << "[WORKER] Waiting worker id and endpoints from server..." << std::endl;
  sock_server.recv(msg, zmq::recv_flags::none);
  uint32_t worker_id = *(const uint32_t*)(msg.data());
  sock_server.recv(msg, zmq::recv_flags::none);
  std::string end_point_pull((char*)msg.data(), msg.size());
  sock_server.recv(msg, zmq::recv_flags::none);
  std::string end_point_push((char*)msg.data(), msg.size());
  
  zmq::socket_t sock_pull(ctx, zmq::socket_type::pull);
  zmq::socket_t sock_push(ctx, zmq::socket_type::push);
  
  sock_pull.connect(end_point_pull);
  sock_push.connect(end_point_push);
  
  std::string log_prefix = "[WORKER " + std::to_string(worker_id) + "] ";
  
  if(log_info) std::cout << log_prefix << "Worker id " << worker_id << " got from server." << std::endl;
    
  std::string data_in;
  std::string data_out;
  
  if(log_info) {
    std::cout << log_prefix << "end_point_pull: " << end_point_pull << std::endl;
    std::cout << log_prefix << "end_point_push: " << end_point_push << std::endl;
  }
  
  while(true) {
    
    // Receive dataset number and its size from server
    if(log_trace) std::cout << log_prefix << "Waiting dataset from server..." << std::endl;
    sock_pull.recv(msg, zmq::recv_flags::none);
    uint32_t dataset = *(const uint32_t*)(msg.data());
    
    if(dataset == -1) {
      if(log_info) std::cout << log_prefix << "Received dataset -1 from server, stopping..." << std::endl;
      sock_push.send(zmq::buffer(&dataset, sizeof(uint32_t)), zmq::send_flags::dontwait);
      break;
    }
    
    sock_pull.recv(msg, zmq::recv_flags::none);
    uint32_t length = *(const uint32_t*)(msg.data());
    if(log_trace) std::cout << log_prefix << "Got dataset " << dataset << " from server." << std::endl;
    
    // Resize input buffer, shall only be once per size
    data_in.resize(length);
    zmq::mutable_buffer buffer_in((void*)data_in.data(), data_in.size());
    
    // Receive data from server
    if(log_trace) std::cout << log_prefix << "Waiting for data from server..." << std::endl;
    sock_pull.recv(buffer_in, zmq::recv_flags::none);
    if(log_trace) std::cout << log_prefix << "Got data from server." << std::endl;

    // Do some processing on data
    data_out.resize(data_in.size());
    
    processDataset(data_in.data(), data_in.size(), (void*)data_out.data(), data_out.size());
    
    length = data_out.size();
    zmq::const_buffer buffer_out((void*)data_out.data(), data_out.size());
        
    // Send data to server
    
    if(log_trace) std::cout << log_prefix << "Posting data " << dataset << " to server..." << std::endl;
    
    sock_push.send(zmq::buffer(&dataset, sizeof(uint32_t)), zmq::send_flags::sndmore);
    sock_push.send(zmq::buffer(&length, sizeof(uint32_t)), zmq::send_flags::sndmore);
    sock_push.send(buffer_out, zmq::send_flags::none);
    
    if(log_trace) std::cout << log_prefix << "Data posted to server." << std::endl;
  }
  
  sock_pull.close();
  sock_push.close();
  
  return 0;
}
