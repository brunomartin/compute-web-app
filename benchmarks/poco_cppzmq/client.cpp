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
  std::string server_address = "tcp://127.0.0.1";
  uint16_t server_port = 5555;
  
  // First argument is server address
  if(argc > 1) {
    server_address = argv[1];
  }
  
  std::string server_end_point = server_address + ":" + std::to_string(server_port);
  
  zmq::context_t ctx;
  zmq::socket_t sock_server(ctx, zmq::socket_type::req);
  
  zmq::message_t msg;
  
  if(log_info) std::cout << "[WORKER] Connect to server " << server_end_point << std::endl;
  sock_server.connect(server_end_point);
    
  // Send worker address to server
  sock_server.send(zmq::message_t(std::string(server_address)), zmq::send_flags::none);

  // server reply contains worker id as a uint32
  if(log_info) std::cout << "[WORKER] Waiting worker id and endpoints from server..." << std::endl;
  sock_server.recv(msg, zmq::recv_flags::none);
  uint32_t worker_id = *(const uint32_t*)(msg.data());
  sock_server.recv(msg, zmq::recv_flags::none);
  std::string end_point_out((char*)msg.data(), msg.size());
  sock_server.recv(msg, zmq::recv_flags::none);
  std::string end_point_in((char*)msg.data(), msg.size());
  
  zmq::socket_t sock_in(ctx, zmq::socket_type::pull);
  zmq::socket_t sock_out(ctx, zmq::socket_type::push);
  
  sock_in.connect(end_point_in);
  sock_out.connect(end_point_out);
  
  std::string log_prefix = "[WORKER " + std::to_string(worker_id) + "] ";
  
  if(log_info) std::cout << log_prefix << "Worker id " << worker_id << " got from server." << std::endl;
    
  std::string data_in;
  std::string data_out;
  
  if(log_info) {
    std::cout << log_prefix << "end_point_in: " << end_point_in << std::endl;
    std::cout << log_prefix << "end_point_out: " << end_point_out << std::endl;
  }
  
  std::string argument((const char*)msg.data(), msg.size());
  
  while(true) {
    // Send a message to server so that it sends data
    msg = zmq::message_t(std::string("GET"));
    
    sock_out.send(msg, zmq::send_flags::none);
    
    // Receive dataset number and its size from server
    if(log_trace) std::cout << log_prefix << "Waiting dataset from server..." << std::endl;
    sock_in.recv(msg, zmq::recv_flags::none);
    uint32_t dataset = *(const uint32_t*)(msg.data());
    sock_in.recv(msg, zmq::recv_flags::none);
    uint32_t length = *(const uint32_t*)(msg.data());
    if(log_trace) std::cout << log_prefix << "Got dataset " << dataset << " from server." << std::endl;
    
    if(dataset == -1) {
      if(log_info) std::cout << log_prefix << "Received dataset -1 from server, stopping..." << std::endl;
      break;
    }
    
    // Resize input buffer, shall only be once per size
    data_in.resize(length);
    zmq::mutable_buffer buffer_in((void*)data_in.data(), data_in.size());
    
    // Receive data from server
    if(log_trace) std::cout << log_prefix << "Waiting for data from server..." << std::endl;
    sock_in.recv(buffer_in, zmq::recv_flags::none);
    if(log_trace) std::cout << log_prefix << "Got data from server." << std::endl;

    // Do some processing on data
    data_out.resize(data_in.size());
    
    processDataset(data_in.data(), data_in.size(), (void*)data_out.data(), data_out.size());
    
    length = data_out.size();
    zmq::const_buffer buffer_out((void*)data_out.data(), data_out.size());
        
    // Send data to server
    
    if(log_trace) std::cout << log_prefix << "Posting data " << dataset << " to server..." << std::endl;
    std::string post_str = "POST " + std::to_string(dataset);
    post_str += " " + std::to_string(data_out.size());
//    sock_out.send(zmq::message_t(post_str), zmq::send_flags::sndmore);
//    sock_out.send(buffer_out, zmq::send_flags::none);
    
    
    sock_out.send(zmq::buffer(std::string("POST")), zmq::send_flags::sndmore);
    sock_out.send(zmq::buffer(&dataset, sizeof(uint32_t)), zmq::send_flags::sndmore);
    sock_out.send(zmq::buffer(&length, sizeof(uint32_t)), zmq::send_flags::sndmore);
    sock_out.send(buffer_out, zmq::send_flags::none);
    
    if(log_trace) std::cout << log_prefix << "Data posted to server." << std::endl;
  }
  
  sock_in.close();
  sock_out.close();
  
  return 0;
}
