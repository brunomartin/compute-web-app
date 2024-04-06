#ifndef CWA_DATA_DATASET_H
#define CWA_DATA_DATASET_H

#include <memory>
#include <string>
#include <vector>

namespace cwa {
namespace data {

class Data;

class Dataset {
public:
  Dataset();
  
  /**
   Set id of the dataset as a string
   For a file, it will be its name, for a database record
   it will be its unique id
   @param id id of the datset
   */
  void SetId(const std::string & id);
  
  /**
   Set data whose this dataset is part, for a file it will be a directory
   For a database, it will be a table
   @param data Data pointer as a shared pointer
   */
  void SetData(const Data* data);
  
  const std::string & GetId() const;
  size_t GetSize() const;
  
  std::string GetFilename() const;
  
  void RetrieveBufferFromFile(const std::string & filename) const;
  
  void RetrieveBuffer(std::string & buffer) const;
  
  void RetainBuffer(std::string & buffer);
  std::string & GetBuffer() const;
  void ReleaseBuffer();
  
  void Persist();

private:
  class Private;
  std::shared_ptr<Private> data_;
};

typedef std::shared_ptr<Dataset> DatasetPtr;
typedef std::shared_ptr<const Dataset> ConstDatasetPtr;
typedef std::vector<DatasetPtr> Datasets;
typedef std::vector<ConstDatasetPtr> ConstDatasets;

}
}

#endif // CWA_DATA_DATASET_H
