#include <memory>
#include <fstream>
#include <vector>
#include <thread>
#include <cstring>
#include <iostream>

#include <log/log.h>

#include <process/exception.h>
#include <process/parameter.h>
#include <process/definition.h>

using cwa::Log;
using cwa::LogLevel;

using cwa::process::Parameter;
using cwa::process::ParameterMap;
using cwa::process::Variant;
using cwa::process::Variants;
using cwa::process::VariantMap;

using cwa::process::Definition;
using cwa::process::BadConversionException;

int TestFailed() {
  Log::Print(LogLevel::Info, "TEST FAILED");
  return -1;
}

int main(int argc, char* argv[]) {
  
  Log::Init(argc, argv);
  Log::Print(LogLevel::Info, "Definition test started");
  
  Definition definition("");
  
  definition.AddParameter(Parameter({
    .name = "test-integer",
    .description = "test integer",
    .default_value = 5
  }));
  
  definition.AddParameter(Parameter({
    .name = "test-decimal",
    .description = "test decimal",
    .default_value = 43.5
  }));
  
  definition.AddParameter(Parameter({
    .name = "test-string",
    .description = "test string",
    .default_value = "--file"
  }));
  
  definition.AddParameter(Parameter({
    .name = "test-vector",
    .description = "test vector",
    .default_value = Variants({42, 1e-6, "test"})
  }));
  
  VariantMap default_values = definition.GetDefaultParameterValues();
  
  if(default_values.size() != 4) TestFailed();
  
  try {
    if(default_values.find("test-integer") == default_values.end()) TestFailed();
    if(default_values["test-integer"].GetType() != Variant::Type::Integer) TestFailed();
    if(default_values["test-integer"].ToInteger() != 5) TestFailed();
    
    if(default_values.find("test-decimal") == default_values.end()) TestFailed();
    if(default_values["test-decimal"].GetType() != Variant::Type::Decimal) TestFailed();
    if(default_values["test-decimal"].ToDecimal() != 43.5) TestFailed();
    
    if(default_values.find("test-string") == default_values.end()) TestFailed();
    if(default_values["test-string"].GetType() != Variant::Type::String) TestFailed();
    if(default_values["test-string"].ToString() != "--file") TestFailed();
    
    if(default_values.find("test-vector") == default_values.end()) TestFailed();
    if(default_values["test-vector"].GetType() != Variant::Type::Variants) TestFailed();
    if(default_values["test-vector"].ToVariants() != Variants({42, 1e-6, "test"})) TestFailed();
  } catch (BadConversionException&) {
    TestFailed();
  }
  
    
  // Now test values one by one
    
  Log::Print(LogLevel::Info, "TEST PASSED");  
  return 0;
}
