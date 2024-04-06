#ifndef CWA_PROCESS_DEFINITION_H
#define CWA_PROCESS_DEFINITION_H

#include <string>
#include <vector>

#include "process/parameter.h"

namespace cwa {
namespace process {

class Definition {
public:
  Definition(const std::string & command, const std::vector<std::string> args = {});
  
  void AddParameter(const Parameter & parameter);
  
  VariantMap GetDefaultParameterValues() const;
  
  const std::string & GetCommand() const;
  const std::vector<std::string> & GetArgs() const;
  
private:
  std::string command_;
  std::vector<std::string> args_;
  ParameterMap parameters_;
  
};

}

}

#endif // CWA_PROCESS_DEFINITION_H
