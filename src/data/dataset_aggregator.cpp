#include "data/dataset_aggregator.h"

namespace cwa {
namespace data {

class DatasetAggregator::Private {
public:
  int id;
  static int next_id;
  int total_buffers = 0;
  int timeout_ms = 5000;
};

int DatasetAggregator::Private::next_id = 1;

DatasetAggregator::DatasetAggregator() : data_(new Private) {
  data_->id = data_->next_id;
  data_->next_id++;
}

int DatasetAggregator::GetId() const {
  return data_->id;
}

void DatasetAggregator::SetTimeout(int timeout_ms) {
  data_->timeout_ms = timeout_ms;
}

int DatasetAggregator::GetTimeout() const {
  return data_->timeout_ms;
}

}
}
