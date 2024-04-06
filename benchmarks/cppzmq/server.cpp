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
  zmq::socket_t sock1(ctx, zmq::socket_type::pull);
  zmq::socket_t sock2(ctx, zmq::socket_type::push);
  
  if(false) {
    sock1.bind("ipc:///tmp/feeds/1");
    sock2.connect("ipc:///tmp/feeds/2");
  } else {
    sock1.bind("tcp://127.0.0.1:5555");
    sock2.connect("tcp://127.0.0.1:5556");
  }

  std::ofstream file("test.raw");

  std::vector<uint16_t> data;
  data.resize(5*1024*1024);
  for(int i=0;i<data.size();i++) {
      data[i] = uint16_t(i);
  }

  file.write(reinterpret_cast<const char*>(&data[0]),
    data.size()*sizeof(uint16_t));

  file.close();
  
  while(true) {
    
//    std::cout << "Waiting for msg..." << std::endl;
    
    zmq::message_t msg;
    
    sock1.recv(msg, zmq::recv_flags::none);
    
//    std::cout << "received msg:" << msg << std::endl;
    
    auto start = high_resolution_clock::now();
    
    if (true) {
      std::ifstream file ("test.raw", std::ifstream::binary);

      // get length of file:
      file.seekg (0, file.end);
      int length = file.tellg();
      file.seekg (0, file.beg);

      // allocate memory:
      std::string buffer;
      buffer.resize(length);

      // read data as a block:
      file.read((char*)buffer.data(), length);

      file.close();
      
      msg = zmq::message_t(std::move(buffer));
    }
    
  //  sock2.send(msg, zmq::send_flags::dontwait);
    sock2.send(msg, zmq::send_flags::none);

    int duration_us = duration_cast<microseconds>(high_resolution_clock::now() - start).count();
    std::cout << "duration_us: " << duration_us << std::endl;
    
  }
  
  
  return 0;
}
