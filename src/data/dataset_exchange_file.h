#ifndef CWA_DATA_DATASET_EXCHANGE_FILE_H
#define CWA_DATA_DATASET_EXCHANGE_FILE_H

#include <string>
#include <fstream>

namespace cwa {
namespace data {

class DatasetExchangeFile {
public:
  DatasetExchangeFile(const std::string & filename);
  ~DatasetExchangeFile();
  
  struct Dataset {
    uint32_t id = -1;
    std::string filename = "";
    uint32_t client_id = -1;
  };
    
  const std::string & GetFilename() const;
  
  void WaitForFile(int timeout_ms = 5000) const;
  
  void LockFile();
  void UnlockFile();
  
  void Init(bool is_aggregator);
  uint32_t RegisterClient();
  
  void PushDataset(const Dataset & dataset);
  
  bool HasDataset() const;
  Dataset GetFirstDataset();
  void PopFirstDataset();
  
  void UnregisterClient(uint32_t client_id);
  
  bool AreAllClientsUnregistered() const;
  
  void RemoveFiles();
  
private:
  std::string filename_;
};

}
}

#endif // CWA_DATA_DATASET_EXCHANGE_FILE_H
