#ifndef CWA_PROCESS_PARAMETER_H
#define CWA_PROCESS_PARAMETER_H

#include <string>
#include <cstdint>
#include <vector>
#include <map>

#include "process/variant.h"

namespace cwa {
namespace process {

class Parameter {
public:
  const std::string name;
  const std::string description;
  const Variant default_value;
  
  Parameter & operator=(const Parameter & rvalue);

private:
  
};

typedef std::map<std::string, Parameter> ParameterMap;

}
}

#endif // CWA_PROCESS_PARAMETER_H
