#include "parameter.h"

namespace cwa {
namespace process {


Parameter & Parameter::operator=(const Parameter & rvalue) {
  Parameter lvalue = {
    .name = rvalue.name,
    .description = rvalue.description,
    .default_value = rvalue.default_value,
  };
  
  return *this = lvalue;
}

}
}
