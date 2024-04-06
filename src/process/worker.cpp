#include "process/worker.h"

namespace cwa {
namespace process {

class Worker::Private {
public:
  Definition definition = Definition("");
  VariantMap parameter_values;
  DatasetAggregatorServerPtr aggregator;
  DatasetSupplierServerPtr supplier;
};

Worker::Worker(const Definition & definition, const VariantMap & parameter_values,
  DatasetSupplierServerPtr supplier, DatasetAggregatorServerPtr aggregator) :
  data_(new Private) {
    data_->definition = definition;
    data_->parameter_values = parameter_values;
    data_->aggregator = aggregator;
    data_->supplier = supplier;
}

const Definition & Worker::GetDefinition() {
  return data_->definition;
}

const VariantMap & Worker::GetParameterValues() {
  return data_->parameter_values;
}

DatasetSupplierServerPtr Worker::GetSupplier() {
  return data_->supplier;
}

DatasetAggregatorServerPtr Worker::GetAggregator() {
  return data_->aggregator;  
}


}
}
