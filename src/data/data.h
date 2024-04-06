#ifndef CWA_DATA_DATA_H
#define CWA_DATA_DATA_H

#include <memory>
#include <vector>

#include "data/attribute.h"
#include "data/dataset.h"

namespace cwa {
namespace data {

typedef std::vector<std::shared_ptr<Attribute>> Attributes;

class Data {
public:
  Data(const std::string & id);
  
  const std::string & GetId() const;
  
  /**
   Add dataset to data
   */
  void AddDataset(DatasetPtr dataset);
  
  const ConstDatasets & GetDatasets() const;

private:
  class Private;
  std::shared_ptr<Private> data_;
};

typedef std::shared_ptr<Data> DataPtr;
typedef std::shared_ptr<const Data> ConstDataPtr;

}
}

#endif // CWA_DATA_DATA_H
