// To build:
// g++-8 server.cpp -I . -o build/server

// #define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>
#include <fstream>
using namespace httplib;

int main(void) {
  Server svr;

  std::ofstream file("test.raw", std::ios::trunc);

  std::vector<uint16_t> data;
  data.resize(5*1024*1024);
  for(int i=0;i<data.size();i++) {
      data[i] = uint16_t(i);
  }

  file.write(reinterpret_cast<const char*>(&data[0]),
    data.size()*sizeof(uint16_t));

  file.close();

  svr.Get("/hi", [](const Request & /*req*/, Response &res) {
    res.set_content("Hello World!", "text/plain");
  });
  
  svr.Get("/file", [](const Request & /*req*/, Response &res) {

    std::ifstream file ("test.raw", std::ifstream::binary);
    if (file) {
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
      
      res.set_content(std::move(buffer), "application/octet-stream");
    }

  });
  
  svr.Post("/file", [](const Request & req, Response &res) {

    std::ifstream file ("test.raw", std::ifstream::binary);
    if (file) {
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
      
      res.set_content(std::move(buffer), "application/octet-stream");
    }

  });
  

  svr.listen("localhost", 8080);
}
