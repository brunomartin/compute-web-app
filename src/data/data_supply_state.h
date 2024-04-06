#ifndef CWA_DATA_PROVIDE_STATE_H
#define CWA_DATA_PROVIDE_STATE_H

#include <vector>

# include <log/log.h>

#include "data/data.h"
#include "data/dataset.h"

namespace cwa {
namespace data {

class DataSupplyState {
public:
  DataSupplyState(ConstDataPtr data);
  
  void GrabDataset(ConstDatasetPtr & dataset, uint32_t & index);

private:
  class Private;
  std::shared_ptr<Private> data_;
};

typedef std::shared_ptr<DataSupplyState> DataSupplyStatePtr;

}
}

#endif // CWA_DATA_PROVIDE_STATE_H
