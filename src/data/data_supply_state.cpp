#include "data/data_supply_state.h"

#include <queue>
#include <mutex>

#include "data/data_exception.h"

namespace cwa {
namespace data {

class DataSupplyState::Private {
public:
  ConstDataPtr data;
  std::queue<uint32_t> datasets_to_provide;
  std::mutex mutex;
};
  
DataSupplyState::DataSupplyState(ConstDataPtr data) : data_(new Private) {
  data_->data = data;
  
  for(uint32_t i=0;i<data_->data->GetDatasets().size();i++) {
    data_->datasets_to_provide.push(i);
  }
}

void DataSupplyState::GrabDataset(ConstDatasetPtr & dataset, uint32_t & index) {
  std::unique_lock<std::mutex> lock(data_->mutex);
  
  if(data_->datasets_to_provide.empty()) {
    throw NoMoreDatasetException();
  }
  
  index = data_->datasets_to_provide.front();
  data_->datasets_to_provide.pop();
  lock.unlock();
  
  ConstDatasets datasets = data_->data->GetDatasets();
  dataset = datasets[index];
}
  
}
}
