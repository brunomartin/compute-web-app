#include <string>
#include <memory>
#include <vector>

#include <data/dataset.h>

#include <process/variant.h>

using cwa::data::DatasetPtr;
using cwa::data::ConstDatasetPtr;

using cwa::process::VariantMap;

class TestWorker {
public:
  TestWorker(const VariantMap & parameters);
  
  static VariantMap ParseArguments(int argc, char* argv[]);
  
  void RunFunction(const std::string& sup_type, const std::string& sup_info, const std::string& agg_type, const std::string& agg_info);
  void ComputeDataset(ConstDatasetPtr& input, DatasetPtr& output);

private:
  class Private;
  std::shared_ptr<Private> data_;
};


