#include "definition.h"

namespace cwa {
namespace process {

Definition::Definition(const std::string & command, const std::vector<std::string> args) :
  command_(command), args_(args){
  
}

const std::string & Definition::GetCommand() const {
  return command_;
}

const std::vector<std::string> & Definition::GetArgs() const {
  return args_;
}

void Definition::AddParameter(const Parameter & parameter) {
  parameters_.insert(std::make_pair(parameter.name, parameter));
}

VariantMap Definition::GetDefaultParameterValues() const {
  
  VariantMap values;
  for(const auto & entry : parameters_) {
    const Parameter & parameter = entry.second;
    values.insert(std::make_pair(parameter.name, parameter.default_value));
  }
  
  return values;
}

}
}
