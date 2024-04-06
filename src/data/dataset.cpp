#include "data/dataset.h"

#include <fstream>

#include <log/log.h>

#include "data/data.h"
#include "data/data_exception.h"

using cwa::Log;
using cwa::LogLevel;

namespace cwa {
namespace data {

class Dataset::Private {
public:
  std::string id;
  const Data* data;
  std::string buffer;
};
  
Dataset::Dataset() : data_(new Private) {
  
}

void Dataset::SetId(const std::string &id) {
  data_->id = id;
}

void Dataset::SetData(const Data* data) {
  data_->data = data;
}

const std::string & Dataset::GetId() const {
  return data_->id;
}

size_t Dataset::GetSize() const {
  
  size_t length = -1;
  
  // First check of there an internal buffer
  // then try to read associated file
  if(!data_->buffer.empty()) {
    length = data_->buffer.size();
  } else {
    if(!data_->data) {
      Log::Print(LogLevel::Fatal, "No data specified for this dataset");
      throw OrphanDatasetException();
    }
    
    std::string filename = GetFilename();
    
    std::ifstream file(filename);
    file.seekg(0, file.end);
    length = file.tellg();
    file.close();
  }
  
  return length;
}

std::string Dataset::GetFilename() const {
  std::string filename = data_->data->GetId();
  filename += "/" + data_->id + ".raw";
  return filename;
}

void Dataset::RetrieveBufferFromFile(const std::string & filename) const {
  
  std::ifstream file(filename);
  file.seekg(0, file.end);
  size_t length = file.tellg();
  data_->buffer.resize(length);
  
  file.seekg(0, file.beg);
  file.read((char*)data_->buffer.data(), length);
  file.close();
  
}

void Dataset::RetrieveBuffer(std::string & buffer) const {
  
  // First check of there an internal buffer
  // then try to read associated file
  if(!data_->buffer.empty()) {
    buffer = data_->buffer;
  } else {
    if(!data_->data) {
      Log::Print(LogLevel::Fatal, "No data specified for this dataset");
      throw OrphanDatasetException();
    }
  
    std::string filename = GetFilename();
    
    std::ifstream file(filename);
    file.seekg(0, file.end);
    size_t length = file.tellg();
    buffer.resize(length);
    
    file.seekg(0, file.beg);
    file.read((char*)buffer.data(), length);
    file.close();
  }
  
}

void Dataset::RetainBuffer(std::string & buffer) {
  data_->buffer = buffer;
}

std::string & Dataset::GetBuffer() const {
  return data_->buffer;
}

void Dataset::ReleaseBuffer() {
  data_->buffer.clear();
}

void Dataset::Persist() {
  
  std::string filename = GetFilename();
  
  std::ofstream file(filename);
  file.write(data_->buffer.data(), data_->buffer.size());
  file.close();
}
  
}
}
