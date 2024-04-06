#include "data/dataset_supplier.h"

namespace cwa {
namespace data {

class DatasetSupplier::Private {
public:  
  int id;
  static int next_id;
  int total_buffers = 0;
  int timeout_ms = 5000;
};

int DatasetSupplier::Private::next_id = 1;

DatasetSupplier::DatasetSupplier() : data_(new Private) {
  data_->id = data_->next_id;
  data_->next_id++;
}

int DatasetSupplier::GetId() const {
  return data_->id;
}

void DatasetSupplier::SetTimeout(int timeout_ms) {
  data_->timeout_ms = timeout_ms;
}

int DatasetSupplier::GetTimeout() const {
  return data_->timeout_ms;
}

}
}
