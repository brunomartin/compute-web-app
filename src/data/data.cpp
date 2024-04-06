#include "data/data.h"

namespace cwa {
namespace data {

class Data::Private {
public:
  std::string id;
  Attributes attributes;
  ConstDatasets datasets;
};

Data::Data(const std::string & id) : data_(new Private) {
  data_->id = id;
}

const std::string & Data::GetId() const {
  return data_->id;
}

void Data::AddDataset(DatasetPtr dataset) {
  dataset->SetData(this);
  data_->datasets.push_back(dataset);
}

const ConstDatasets & Data::GetDatasets() const {
  return data_->datasets;
}

}
}
