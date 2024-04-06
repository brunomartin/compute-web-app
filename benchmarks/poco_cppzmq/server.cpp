#include <iostream>
#include <chrono>
#include <fstream>
#include <algorithm>
#include <thread>
#include <regex>
#include <cstdio>
#include <sstream>
#include <queue>
#include <mutex>
#include <condition_variable>

#include <Poco/Process.h>
#include <zmq.hpp>

#include "process.h"

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::microseconds;

using Poco::Process;
using Poco::ProcessHandle;

bool log_trace = false;
bool log_info = false;

std::queue<uint32_t> datasets;
std::mutex datasets_mutex;

void WorkerThread(int worker, std::shared_ptr<zmq::socket_t> sock_in, std::shared_ptr<zmq::socket_t> sock_out, int dataset_count);

void MergeFiles(const std::string & base_filename, int dataset_count);

void ComputeExtectedOutput(const std::string & input_filename, const std::string & output_filename);

int main(int argc, char* argv[]) {
  
  // Set input data properties
  size_t dataset_count = 20;
  size_t dataset_size_b = 1*1024*1024;
  
  // Set number of workers
  size_t worker_count = 2;
  
  // Set connection configuration
  std::string server_address = "tcp://127.0.0.1";
  server_address = "tcp://*";
//  server_address = "tcp://192.168.1.50";
  uint16_t server_port = 5555;
  
  // Set working directory for processes to launch
  std::string working_directory = ".";
//  working_directory = "../Release";
  
  // First argument is number of workers
  if(argc > 1) {
    worker_count = atoi(argv[1]);
  }
  
  // Second argument is number of datasets
  if(argc > 2) {
    dataset_count = atoi(argv[2]);
  }
      
  // Compute total data size
  size_t data_size_b = dataset_count*dataset_size_b;
  
  high_resolution_clock::time_point start;
  int duration_us;
  
  std::cout << "[SERVER] " << int(data_size_b/1024/1024) << "MB = ";
  std::cout << dataset_count << "x" << int(dataset_size_b/1024) << "kB datasets, ";
  if(worker_count == -1) {
    std::cout << "computing alone." << std::endl;
  } else {
    std::cout << worker_count << " workers." << std::endl;
  }
  // Fill dataset queue with indices
  for(uint32_t i=0;i<dataset_count;i++) datasets.push(i);
  
  // Open file and check if a new one is needed
  bool create_file = false;
  std::fstream file("server_input.raw", std::ios::in);
  
  // get length of file and check if creation is needed
  file.seekg(0, file.end);
  create_file = file.tellg() != data_size_b;
  file.close();
  
  if(create_file) {
    // Create data content
    std::cout << "[SERVER] Generating and writing file..." << std::endl;
    std::vector<uint16_t> data;
    data.resize(data_size_b / sizeof(uint16_t));
    for(int i=0;i<data.size();i++) data[i] = uint16_t(i);
    
    // Write intput test file
    std::ofstream file("server_input.raw");
    file.write((const char*)data.data(), data_size_b);
    file.close();
    
    std::cout << "[SERVER] File generated." << std::endl;
  }
  
  if(worker_count == -1) {
    // Compute in main thread for comparison
    start = high_resolution_clock::now();
    ComputeExtectedOutput("server_input.raw", "server_output_expected.raw");
    duration_us = duration_cast<microseconds>(high_resolution_clock::now() - start).count();
    std::cout << "[SERVER] Expected output computed and stored in " << int(duration_us/1000.) << "ms." << std::endl;
    
    return 0;
  }

  start = high_resolution_clock::now();
  
  zmq::context_t ctx;
  zmq::socket_t sock(ctx, zmq::socket_type::rep);
  
  // Bind to address and port, port requires to be free
  // any call to recv will block until a worker sends a
  // message
  sock.bind(server_address + ":" + std::to_string(server_port));
  std::cout << "[SERVER] Start listening on end point ";
  std::cout << sock.get(zmq::sockopt::last_endpoint) << "." << std::endl;
  
  sock.set(zmq::sockopt::rcvtimeo, 2000);
  
  if(log_info) std::cout << "[SERVER] Spawning " << worker_count << " processes..." << std::endl;
  start = high_resolution_clock::now();
  std::vector<ProcessHandle> processes;
  for(int worker=0;worker<std::min(worker_count, size_t(32));worker++) {
    Poco::Process::Args args;
    ProcessHandle process = Process::launch("./client", args, working_directory);
    processes.push_back(std::move(process));
  }
  duration_us = duration_cast<microseconds>(high_resolution_clock::now() - start).count();
  if(log_info) std::cout << "[SERVER] Processes spawned in " << (duration_us/1000) << "ms." << std::endl;
  
  start = high_resolution_clock::now();
  std::vector<std::thread> worker_threads;
  
  int workers=0;
  while(!datasets.empty()) {
    
    // datasets might be updated, yield thread to be sure it is up to date
    std::this_thread::yield();
    
    // Workers shall connect to server to received information for their io threads
    // and sockets
    zmq::message_t msg;
    std::shared_ptr<zmq::socket_t> sock_in, sock_out;
    std::string worker_address;

    try {
      // sock recv has a timeout, if result is false then a timeout occured
      if(!sock.recv(msg, zmq::recv_flags::none)) {
        continue;
      }
      
      worker_address = std::string((char*)msg.data(), msg.size());
      
      sock_in.reset(new zmq::socket_t(ctx, zmq::socket_type::pull));
      sock_out.reset(new zmq::socket_t(ctx, zmq::socket_type::push));
   
      sock_in->bind(worker_address + ":*");
      sock_out->bind(worker_address + ":*");
      
      std::string end_point_in = sock_in->get(zmq::sockopt::last_endpoint);
      std::string end_point_out = sock_out->get(zmq::sockopt::last_endpoint);
      
      if(log_info) std::cout << "[SERVER] Sending worker id " << workers << " to worker..." << std::endl;
      sock.send(zmq::buffer(&workers, sizeof(uint32_t)), zmq::send_flags::sndmore);
      sock.send(zmq::message_t(end_point_in), zmq::send_flags::sndmore);
      sock.send(zmq::message_t(end_point_out), zmq::send_flags::none);
      if(log_info) std::cout << "[SERVER] worker id " << workers << " sent to worker." << std::endl;
      
      duration_us = duration_cast<microseconds>(high_resolution_clock::now() - start).count();
    
      std::cout << "[SERVER] worker " << workers << " registered after ";
      std::cout << int(duration_us/1000) << "ms." << std::endl;
    
      if(log_info) {
        std::cout << "[SERVER] worker " << workers << " worker_address: " << worker_address << std::endl;
        std::cout << "[SERVER] worker " << workers << " end_point_in: " << end_point_in << std::endl;
        std::cout << "[SERVER] worker " << workers << " end_point_out: " << end_point_out << std::endl;
      }
      
      // Once socket are created, create a thread for exchanging data
      std::thread thread = std::thread(WorkerThread, workers, sock_in, sock_out, dataset_count);
      worker_threads.push_back(std::move(thread));
      
      workers++;
      
      if(worker_count != 0 && workers >= worker_count) {
        break;
      }
      
    } catch(zmq::error_t & e) {
      std::cout << "[SERVER] zmq::error_t " << e.what() << std::endl;
      continue;
    }
  }
  
  // Wait for working thread to finish
  for(std::thread & thread : worker_threads) {
    thread.join();
  }
  
  duration_us = duration_cast<microseconds>(high_resolution_clock::now() - start).count();
  std::cout << "[SERVER] total duration: " << int(duration_us/1000.) << "ms." << std::endl;
  
  // Wait for worker processes to finish
  for(auto process : processes) {
    process.wait();
  }
  
  std::cout << "[SERVER] workers gracefully finished." << std::endl;
  
  // merge server output data too
  if(workers > 0) MergeFiles("server_output", dataset_count);
  
  return 0;
}

void WorkerThread(int worker, std::shared_ptr<zmq::socket_t> sock_in, std::shared_ptr<zmq::socket_t> sock_out, int dataset_count) {
  
  zmq::message_t msg;
  std::string data_in;
  std::string data_out;
  
  // Then wait for command and handle it
  while(true) {
    
    if(log_trace) std::cout << "[SERVER] waiting command from client " << worker << "..." << std::endl;
    sock_in->recv(msg, zmq::recv_flags::none);
    if(log_trace) std::cout << "[SERVER] client " << worker << " command: " << msg << std::endl;
    
    std::string command = std::string(static_cast<char*>(msg.data()), msg.size());
    
    // Use regular expression to catch command name
    std::regex regex("^(GET|POST)$");
    std::smatch matches;
    if(!std::regex_match(command, matches, regex)) {
      std::cerr << "[SERVER] Command not handled: " << command << std::endl;
      continue;
    }
    
    // Get command name as a string
    command = matches[1];
    
    uint32_t dataset = -1;
    uint32_t length = -1;
    
    if(command == "GET") {
      
      std::unique_lock<std::mutex> lock(datasets_mutex);
      if(datasets.empty()) {
        // Send -1 to tell worker that there is no more dataset to process
        sock_out->send(zmq::buffer(&dataset, sizeof(uint32_t)), zmq::send_flags::sndmore);
        sock_out->send(zmq::buffer(&length, sizeof(uint32_t)), zmq::send_flags::dontwait);
        break;
      }
      dataset = datasets.front();
      datasets.pop();
      lock.unlock();
      
      std::ifstream file ("server_input.raw");

      // get length of file and compute dataset length
      file.seekg(0, file.end);
      length = file.tellg();
      length /= dataset_count;

      // allocate memory and read data
      data_out.resize(length);
      file.seekg(length*dataset, file.beg);
      file.read((char*)data_out.data(), length);
      file.close();
      
      zmq::const_buffer buffer_out(data_out.data(), data_out.size());
      
      sock_out->send(zmq::buffer(&dataset, sizeof(uint32_t)), zmq::send_flags::sndmore);
      sock_out->send(zmq::buffer(&length, sizeof(uint32_t)), zmq::send_flags::sndmore);
      sock_out->send(buffer_out, zmq::send_flags::none);
      
    } else if(command == "POST") {
      
      // After POST command, client sends dataset number and length
      sock_in->recv(msg, zmq::recv_flags::none);
      dataset = *(const uint32_t*)(msg.data());
      sock_in->recv(msg, zmq::recv_flags::none);
      length = *(const uint32_t*)(msg.data());
      
      data_in.resize(length);
      
      // Next message contains data
      if(log_trace) std::cout << "[SERVER] waiting for client " << worker << "..." << std::endl;
      zmq::mutable_buffer buffer_in((void*)data_in.data(), data_in.size());
      sock_in->recv(buffer_in, zmq::recv_flags::none);
      if(log_trace) std::cout << "[SERVER] client " << worker << " posted data " << dataset << std::endl;
            
      // write the whole message content in file
      std::string filename ="server_output_" + std::to_string(dataset) + ".raw";
      std::ofstream file(filename, std::ifstream::binary);
      file.write((const char*)data_in.data(), data_in.size());
      file.close();
    } else {
      std::cerr << "[SERVER] worker " << worker << " Command not handled: " << command << std::endl;
    }
        
  }
}

void ComputeExtectedOutput(const std::string & input_filename, const std::string & output_filename) {
  
  std::ifstream file (input_filename, std::ifstream::binary);

  // get length of file:
  file.seekg(0, file.end);
  int length = file.tellg();
  file.seekg(0, file.beg);
  
  // allocate memory and read data
  std::string data_in;
  data_in.resize(length);
  file.read((char*)data_in.data(), length);
  
  // Compute expected output
  std::string data_out;
  data_out.resize(data_in.size());
  
  processDataset(data_in.data(), data_in.size(), (void*)data_out.data(), data_out.size());
    
  // Write expected output file
  std::ofstream file_out("server_output_expected.raw");
  file_out.write((const char*)data_out.data(), data_out.size());
  file_out.close();

  file.close();
}

void MergeFiles(const std::string & base_filename, int dataset_count) {
    
  std::ifstream base_file (base_filename + "_0.raw", std::ifstream::binary);
  base_file.seekg(0, base_file.end);
  int length = base_file.tellg();
  base_file.seekg(0, base_file.beg);
  base_file.close();
  
  std::string data;
  data.resize(length*dataset_count);
  for(size_t i=0;i<dataset_count;i++) {
    size_t offset = length*i;
    char* global_data = (char*)(data.data() + offset);
    
    std::string filename = base_filename + "_" + std::to_string(i) + ".raw";
    std::ifstream file(filename, std::ifstream::binary);
    file.read(global_data, length);
    file.close();
    
    remove(filename.data());
  }
  
  // and stored to client input file
  std::ofstream file(base_filename + ".raw", file.binary);
  file.write((char*)data.data(), data.size());
  file.close();
}
