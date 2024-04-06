#include <memory>
#include <fstream>
#include <vector>
#include <thread>
#include <cstring>

#include <log/log.h>

#include <process/exception.h>
#include <process/parameter.h>

using cwa::Log;
using cwa::LogLevel;

using cwa::process::Parameter;
using cwa::process::Variant;
using cwa::process::Variants;
using cwa::process::BadConversionException;

int TestFailed() {
  Log::Print(LogLevel::Info, "TEST FAILED");
  return -1;
}

int main(int argc, char* argv[]) {
  
  Log::Init(argc, argv);
  Log::Print(LogLevel::Info, "Parameter test started");
    
  Parameter parameter_integer = {
    .name = "test-integer",
    .description = "test integer",
    .default_value = 5
  };
  
  Parameter parameter_decimal = {
    .name = "test-decimal",
    .description = "test decimal",
    .default_value = 43.5
  };
  
  Parameter parameter_string = {
    .name = "test-string",
    .description = "test string",
    .default_value = "--file"
  };
  
  Parameter parameter_variants = {
    .name = "test-vector",
    .description = "test vector",
    .default_value = Variants({42, 1e-6, "test"})
  };
    
  if(parameter_integer.default_value.GetType() != Variant::Type::Integer) return TestFailed();
  if(parameter_integer.default_value.ToInteger() != 5) return TestFailed();
  
  if(parameter_decimal.default_value.GetType() != Variant::Type::Decimal) return TestFailed();
  if(parameter_decimal.default_value.ToDecimal() != 43.5) return TestFailed();
  
  if(parameter_string.default_value.GetType() != Variant::Type::String) return TestFailed();
  if(parameter_string.default_value.ToString() != "--file") return TestFailed();
  
  if(parameter_variants.default_value.GetType() != Variant::Type::Variants) return TestFailed();
  Variants vector = parameter_variants.default_value.ToVariants();
  
  if(vector.size() != 3) return TestFailed();
  
  if(vector[0].GetType() != Variant::Type::Integer) return TestFailed();
  if(vector[0].ToInteger() != 42) return TestFailed();
  
  if(vector[1].GetType() != Variant::Type::Decimal) return TestFailed();
  if(vector[1].ToDecimal() != 1e-6) return TestFailed();
  
  if(vector[2].GetType() != Variant::Type::String) return TestFailed();
  if(vector[2].ToString() != "test") return TestFailed();
  
  vector[0] = 45.5;
  if(vector[0].GetType() != Variant::Type::Decimal) return TestFailed();
  if(vector[0].ToDecimal() != 45.5) return TestFailed();
  
  vector[0] = vector[1];
  
  if(vector[0].GetType() != Variant::Type::Decimal) return TestFailed();
  if(vector[0].ToDecimal() != 1e-6) return TestFailed();
  
  if(vector[1].GetType() != Variant::Type::Decimal) return TestFailed();
  if(vector[1].ToDecimal() != 1e-6) return TestFailed();
  
  Log::Print(LogLevel::Info, "TEST PASSED");
  
  return 0;
}
