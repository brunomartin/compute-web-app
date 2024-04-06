#include <zmq.hpp>
#include <iostream>
#include <chrono>
#include <fstream>

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::microseconds;

int main()
{
  zmq::context_t ctx;
  zmq::socket_t sock1(ctx, zmq::socket_type::push);
  zmq::socket_t sock2(ctx, zmq::socket_type::pull);
  
  if(false) {
    sock1.connect("ipc:///tmp/feeds/1");
    sock2.bind("ipc:///tmp/feeds/2");
  } else {
    sock1.connect("tcp://127.0.0.1:5555");
    sock2.bind("tcp://127.0.0.1:5556");
  }

  zmq::message_t msg;
  msg.rebuild(5*1024*1024);
  
  double mean_duration_us = 0.;
  int count = 100;
  
  for(int i=0;i<count;i++) {
    
    auto start = high_resolution_clock::now();
    
//    std::cout << "Sending msg..." << std::endl;
    
    sock1.send(zmq::message_t(), zmq::send_flags::dontwait);
  //  sock1.send(msg, zmq::send_flags::none);
    
//    std::cout << "Waiting for msg..." << std::endl;
    
//    start = high_resolution_clock::now();
    
    sock2.recv(msg, zmq::recv_flags::none);
    
    int duration_us = duration_cast<microseconds>(high_resolution_clock::now() - start).count();
    mean_duration_us += duration_us;
  }  
  
  std::cout << "msg: " << msg << std::endl;  
  
  std::ofstream file("result.raw", std::ios::binary);
  file.write(reinterpret_cast<const char*>(msg.data()), msg.size());
  file.close();
  
  mean_duration_us /= count;
  
  std::cout << "mean_duration_us: " << mean_duration_us << std::endl;
  
  return 0;
}
