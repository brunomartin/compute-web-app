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
#include <Poco/Pipe.h>
#include <Poco/PipeStream.h>
#include <zmq.hpp>

#include "process.h"

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::microseconds;

using Poco::Process;
using Poco::ProcessHandle;

bool log_trace = false;
bool log_info = false;

std::queue<uint32_t> available_datasets;
std::queue<uint32_t> processed_datasets;
std::mutex datasets_mutex;
std::mutex output_file_mutex;

void ServerReplyThread(const std::string server_address, bool & stop, int dataset_count);

void WorkerPushThread(int worker, std::shared_ptr<zmq::socket_t> sock_push, std::shared_ptr<std::mutex> mutex, std::shared_ptr<std::condition_variable> cv, std::shared_ptr<bool> is_processing, int dataset_count);
void WorkerPullThread(int worker, std::shared_ptr<zmq::socket_t> sock_pull, std::shared_ptr<std::mutex> mutex, std::shared_ptr<std::condition_variable> cv, std::shared_ptr<bool> is_processing, int dataset_count);

void ComputeExtectedOutput(const std::string & input_filename, const std::string & output_filename, int dataset_count);

int main(int argc, char* argv[]) {
  
  // Set input data properties
  size_t dataset_count = 10;
  size_t dataset_size_b = 4*1024*1024;
  
  // Set number of workers
  size_t worker_count = 2;
  
  // Set connection configuration
  std::string server_host = "tcp://*";
  uint16_t server_port = 5555;
  
  Poco::Process::Args args;
  
  // Set working directory and command line to start clients
  std::string working_directory = ".";
  std::string client_command;
  
  // Start C/C++ client
  client_command = "./client";
  
  // Start python client
//  client_command = "source .venv/bin/activate && python client.py";
    
  std::string server_address = server_host + ":" + std::to_string(server_port);
  
  // Uncomment in order to test ipc for communication
//  server_address = "ipc:///tmp/cwa_server";
  
  // Set working directory for processes to launch
  
  // First argument is number of workers
  if(argc > 1) {
    worker_count = atoi(argv[1]);
  }
  
  // Second argument is number of datasets
  if(argc > 2) {
    dataset_count = atoi(argv[2]);
  }
  
  // Third argument is size of datasets in MB
  if(argc > 3) {
    dataset_size_b = atof(argv[3])*1024*1024;
  }
      
  // Compute total data size
  size_t data_size_b = dataset_count*dataset_size_b;
    
  std::cout << "[SERVER] " << int(data_size_b/1024/1024) << "MB = ";
  std::cout << dataset_count << "x" << int(dataset_size_b/1024) << "kB datasets, ";
  if(worker_count == -1) {
    std::cout << "computing alone." << std::endl;
  } else {
    std::cout << worker_count << " workers." << std::endl;
  }
  // Fill dataset queue with indices
  for(uint32_t i=0;i<dataset_count;i++) available_datasets.push(i);
  
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
  
  auto start = high_resolution_clock::now();
  
  if(worker_count == -1) {
    // Compute in main thread for comparison
    ComputeExtectedOutput("server_input.raw", "server_output_expected.raw", dataset_count);
    int duration_us = duration_cast<microseconds>(high_resolution_clock::now() - start).count();
    std::cout << "[SERVER] total duration: " << int(duration_us/1000.) << "ms." << std::endl;
    return 0;
  }
  
  start = high_resolution_clock::now();
  
  // Create a process list, it will be filled in thread in charge of spwaning processes
  std::vector<ProcessHandle> processes;
  
  // Spawn threads in thread aside to be able to get output without blocking main thread
  std::thread process_thread = std::thread([worker_count, working_directory, client_command, &processes]() {
    
    std::vector<Poco::Pipe> out_pipes;
    
    if(log_info) std::cout << "[SERVER] Spawning " << worker_count << " processes..." << std::endl;
    for(int worker=0;worker<std::min(worker_count, size_t(32));worker++) {
          
      Poco::Process::Args args;
      std::string command = "/bin/sh";
      args.push_back("-c");
      args.push_back(client_command);
      
      Poco::Pipe out_pipe;
      ProcessHandle process = Process::launch(command, args, working_directory, 0, &out_pipe, &out_pipe);
      
      out_pipes.push_back(out_pipe);
      processes.push_back(process);
    }
    if(log_info) std::cout << "[SERVER] Processes spawned." << std::endl;
    
    std::vector<std::thread> out_pipe_threads;
    
    for(int worker=0;worker<std::min(worker_count, size_t(32));worker++) {
      
      // Start thread to listen to output unless one process may block all others
      std::thread out_pipe_thread([worker, &out_pipes]() {
        
        // Read output of process
        // Beware that if process does not output anything, it will block
        Poco::PipeInputStream input_stream(out_pipes[worker]);
        std::string worker_out(std::istreambuf_iterator<char>(input_stream), {});
        std::cout << "[SERVER] worker " << worker << " out: " << worker_out << std::endl;
        
      });
      
      out_pipe_threads.push_back(std::move(out_pipe_thread));
    }
    
    for(std::thread & out_pipe_thread : out_pipe_threads) {
      out_pipe_thread.join();
    }
    
  });
      
  // Trunc file if already exist, pull thread will grow it as they
  // pull data
  std::ofstream file_out("server_output.raw", std::ios::trunc);
  file_out.close();
  
  bool server_thread_stop = false;
  std::thread server_thread(ServerReplyThread, server_address, std::ref(server_thread_stop), dataset_count);
  
  while(processed_datasets.size() < dataset_count) {
    // sleep for a while to yield cpu and not consume too much cpu ressource
    // but not too much to keep a low process duration
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
    
  int duration_us = duration_cast<microseconds>(high_resolution_clock::now() - start).count();
  std::cout << "[SERVER] total duration: " << int(duration_us/1000.) << "ms." << std::endl;
  
  // Stop server thread and wait until it gently finishes
  server_thread_stop = true;
  server_thread.join();
  
  process_thread.join();
  
  // Wait for worker processes to finish
  for(auto process : processes) {
    process.wait();
  }
  
  if(log_info) std::cout << "[SERVER] worker processes gracefully finished." << std::endl;
  
  return 0;
}

void ServerReplyThread(const std::string server_address, bool & stop, int dataset_count) {
  
  auto start = high_resolution_clock::now();
  
  zmq::context_t ctx;
  zmq::socket_t sock(ctx, zmq::socket_type::rep);
  
  // Bind to address and port, port requires to be free
  // any call to recv will block until a worker sends a
  // message
  sock.bind(server_address);
  std::cout << "[SERVER] Start listening on end point ";
  std::cout << sock.get(zmq::sockopt::last_endpoint) << "." << std::endl;
  
  std::vector<std::thread> worker_threads;
  
  int workers=0;
  while(!stop) {
    
    // Workers shall connect to server to receive information for their io threads
    // and sockets
    zmq::message_t msg;
    std::shared_ptr<zmq::socket_t> sock_push, sock_pull;
    std::string worker_address;

    try {
      auto res = sock.recv(msg, zmq::recv_flags::dontwait);
      
      // check if has value, if not wait a while before trying again
      if(!res.has_value()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        continue;
      }
      
      worker_address = std::string((char*)msg.data(), msg.size());
      
      sock_push.reset(new zmq::socket_t(ctx, zmq::socket_type::push));
      sock_pull.reset(new zmq::socket_t(ctx, zmq::socket_type::pull));
      
      if(server_address.substr(0, 3) == "tcp") {
        worker_address += ":*";
        sock_push->bind(worker_address);
        sock_pull->bind(worker_address);
      } else if (server_address.substr(0, 3) == "ipc") {
        std::string ipc_address;
        ipc_address = worker_address + "_" + std::to_string(workers) + "_push";
        sock_push->bind(ipc_address);
        ipc_address = worker_address + "_" + std::to_string(workers) + "_pull";
        sock_pull->bind(ipc_address);
      } else {
        std::cerr << "Bad server address" << std::endl;
        exit(-1);
      }
      
      std::string end_point_push = sock_push->get(zmq::sockopt::last_endpoint);
      std::string end_point_pull = sock_pull->get(zmq::sockopt::last_endpoint);
      
      if(log_info) std::cout << "[SERVER] Sending worker id " << workers << " to worker..." << std::endl;
      sock.send(zmq::buffer(&workers, sizeof(uint32_t)), zmq::send_flags::sndmore);
      sock.send(zmq::message_t(end_point_push), zmq::send_flags::sndmore);
      sock.send(zmq::message_t(end_point_pull), zmq::send_flags::none);
      if(log_info) std::cout << "[SERVER] worker id " << workers << " sent to worker." << std::endl;
      
      int duration_us = duration_cast<microseconds>(high_resolution_clock::now() - start).count();
    
      std::cout << "[SERVER] worker " << workers << " registered after ";
      std::cout << int(duration_us/1000) << "ms." << std::endl;
      
      if(log_info) {
        std::cout << "[SERVER] worker " << workers << " worker_address: " << worker_address << std::endl;
        std::cout << "[SERVER] worker " << workers << " end_point_push: " << end_point_push << std::endl;
        std::cout << "[SERVER] worker " << workers << " end_point_pull: " << end_point_pull << std::endl;
      }
      
      // Create a mutex to avoid pushing data to busy worker
      std::shared_ptr<std::mutex> worker_mutex(new std::mutex);
      std::shared_ptr<std::condition_variable> worker_cv(new std::condition_variable);
      std::shared_ptr<bool> is_processing(new bool(false));
      
      // Once socket are created, create a thread for exchanging data
      worker_threads.push_back(std::thread(WorkerPushThread, workers, sock_push, worker_mutex, worker_cv, is_processing, dataset_count));
      worker_threads.push_back(std::thread(WorkerPullThread, workers, sock_pull, worker_mutex, worker_cv, is_processing, dataset_count));
      
      workers++;
      
    } catch(zmq::error_t & e) {
      std::cout << "[SERVER] zmq::error_t " << e.what() << std::endl;
      continue;
    }
  }
  
  // Wait for working thread to finish
  for(std::thread & thread : worker_threads) {
    thread.join();
  }
  
  
}

void WorkerPushThread(int worker, std::shared_ptr<zmq::socket_t> sock_push, std::shared_ptr<std::mutex> mutex, std::shared_ptr<std::condition_variable> cv, std::shared_ptr<bool> is_processing, int dataset_count) {
  
  std::string data;
  
  while(true) {
    
    uint32_t dataset = -1;
    uint32_t length = -1;
    
    std::unique_lock<std::mutex> datasets_lock(datasets_mutex);
    if(available_datasets.empty()) {
      // Send -1 to tell worker that there is no more dataset to process
      sock_push->send(zmq::buffer(&dataset, sizeof(uint32_t)), zmq::send_flags::none);
      break;
    }
    dataset = available_datasets.front();
    available_datasets.pop();
    datasets_lock.unlock();
    
    std::ifstream file ("server_input.raw");

    // get length of file and compute dataset length
    file.seekg(0, file.end);
    length = file.tellg();
    length /= dataset_count;
    
    // allocate memory and read data
    data.resize(length);
    file.seekg(length*dataset, file.beg);
    file.read((char*)data.data(), length);
    file.close();
    
    // If already processing, wait to push new data
    std::unique_lock<std::mutex> worker_lock(*mutex);
    if(*is_processing) {
      cv->wait(worker_lock);
    }
    *is_processing = true;
    cv->notify_all();
    worker_lock.unlock();
    
    zmq::const_buffer buffer_out(data.data(), data.size());
    
    sock_push->send(zmq::buffer(&dataset, sizeof(uint32_t)), zmq::send_flags::sndmore);
    sock_push->send(zmq::buffer(&length, sizeof(uint32_t)), zmq::send_flags::sndmore);
    sock_push->send(buffer_out, zmq::send_flags::none);
  }
}

void WorkerPullThread(int worker, std::shared_ptr<zmq::socket_t> sock_pull, std::shared_ptr<std::mutex> mutex, std::shared_ptr<std::condition_variable> cv, std::shared_ptr<bool> is_processing, int dataset_count) {
  
  zmq::message_t msg;
  std::string data;
  
  while(true) {
    
    sock_pull->recv(msg, zmq::recv_flags::none);
    uint32_t dataset = *(const uint32_t*)(msg.data());
    
    if(dataset == -1) {
      if(log_info) std::cout << "[SERVER] Received dataset -1 from client, stopping..." << std::endl;
      break;
    }
    
    // If not processing, wait that data has been sent to worker to start waiting
    // for processed data
    std::unique_lock<std::mutex> worker_lock(*mutex);
    if(!*is_processing) {
      cv->wait(worker_lock);
    }
    
    sock_pull->recv(msg, zmq::recv_flags::none);
    uint32_t length = *(const uint32_t*)(msg.data());
    
    data.resize(length);
    
    // Next message contains data
    if(log_trace) std::cout << "[SERVER] waiting for client " << worker << "..." << std::endl;
    zmq::mutable_buffer buffer_in((void*)data.data(), data.size());
    sock_pull->recv(buffer_in, zmq::recv_flags::none);
    if(log_trace) std::cout << "[SERVER] client " << worker << " posted data " << dataset << std::endl;
    
    // Now processed data is received, mark worker as not processing
    *is_processing = false;
    cv->notify_all();
    
    // Unlock it so that new data can be pushed to worker
    worker_lock.unlock();
    
    // write the whole message content in file
    std::lock_guard<std::mutex> lock(output_file_mutex);
    std::string filename ="server_output.raw";
    std::ofstream file_out(filename, std::ios::app);
    file_out.seekp(dataset*length, file_out.beg);
    file_out.write((const char*)data.data(), data.size());
    file_out.close();
    
    processed_datasets.push(dataset);
    
  }
  
}

void ComputeExtectedOutput(const std::string & input_filename, const std::string & output_filename, int dataset_count) {
  
  std::ifstream file_in(input_filename, std::ifstream::binary);

  // get length of file:
  file_in.seekg(0, file_in.end);
  int length = file_in.tellg();
  length /= dataset_count;
  
  // allocate memory and read data
  std::string data_in;
  std::string data_out;
  data_in.resize(length);
  data_out.resize(length);
  
  std::ofstream file_out("server_output_expected.raw", std::ios::trunc);
  file_out.close();
  
  for(int dataset=0;dataset<dataset_count;dataset++) {
    
    file_in.seekg(length*dataset, file_in.beg);
    file_in.read((char*)data_in.data(), length);
    
    // Compute expected output
    processDataset(data_in.data(), data_in.size(), (void*)data_out.data(), data_out.size());
    
    // Write expected output file
    std::string filename ="server_output_expected.raw";
    std::ofstream file_out(filename, std::ios::app);
    file_out.seekp(dataset*length, file_out.beg);
    file_out.write((const char*)data_out.data(), data_out.size());
    file_out.close();
  }
  
  file_in.close();
}
