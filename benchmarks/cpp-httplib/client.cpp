// To build:
// g++-8 server.cpp -I . -o build/server

// #define (CPPHTTPLIB)_OPENSSL_SUPPORT
#include <httplib.h>
#include <vector>
#include <chrono>

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::microseconds;

int main(void) {
  httplib::Client cli("localhost", 8080);
  
  std::string buffer;
  
  auto start = high_resolution_clock::now();
  auto res = cli.Get("/file");
  buffer.assign(std::move(res->body));
  int duration_us = duration_cast<microseconds>(high_resolution_clock::now()-start).count();
  std::cout << "duration_us: " << duration_us << std::endl;
  
  start = high_resolution_clock::now();
  
  std::ifstream file ("test.raw", std::ifstream::binary);
  // get length of file:
  file.seekg (0, file.end);
  int length = file.tellg();
  file.seekg (0, file.beg);

  // allocate memory:
  buffer.resize(length);

  // read data as a block:
  file.read ((char*)buffer.data(), length);

  file.close();
  
  duration_us = duration_cast<microseconds>(high_resolution_clock::now()-start).count();
  std::cout << "duration_us: " << duration_us << std::endl;
  
}
