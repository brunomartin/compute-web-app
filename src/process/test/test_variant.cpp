#include <memory>
#include <fstream>
#include <vector>
#include <thread>
#include <cstring>

#include <log/log.h>

#include <process/exception.h>
#include <process/variant.h>

using cwa::Log;
using cwa::LogLevel;

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
    
  Variant integer_value = 15;
  Variant decimal_value = 14.5;
  Variant string_value = std::string("test");
  Variant c_string_value = "test c string";
  Variant variants_values = Variants{42, 1e-6, "test"};
  
  if(integer_value.GetType() != Variant::Type::Integer) return TestFailed();
  if(integer_value.ToInteger() != 15) return TestFailed();
  
  if(decimal_value.GetType() != Variant::Type::Decimal) return TestFailed();
  if(decimal_value.ToDecimal() != 14.5) return TestFailed();
  
  if(string_value.GetType() != Variant::Type::String) return TestFailed();
  if(string_value.ToString() != "test") return TestFailed();
  
  if(c_string_value.GetType() != Variant::Type::String) return TestFailed();
  if(c_string_value.ToString() != "test c string") return TestFailed();
  
  if(variants_values.GetType() != Variant::Type::Variants) return TestFailed();
  
  Variants variants = variants_values.ToVariants();
  
  if(variants.size() != 3) return TestFailed();
    
  if(variants[0].GetType() != Variant::Type::Integer) return TestFailed();
  if(variants[0].ToInteger() != 42) return TestFailed();
  
  if(variants[1].GetType() != Variant::Type::Decimal) return TestFailed();
  if(variants[1].ToDecimal() != 1e-6) return TestFailed();
  
  if(variants[2].GetType() != Variant::Type::String) return TestFailed();
  if(variants[2].ToString() != "test") return TestFailed();
  
  try {
    variants[0].ToInteger();
  } catch(BadConversionException&) {
    return TestFailed();
    
  }
   
  try {
    variants[0].ToDecimal();
    return TestFailed();
  } catch(BadConversionException&) {}
  
  try {
    variants[0].ToString();
    return TestFailed();
  } catch(BadConversionException&) {}
  
  try {
    variants[0].ToVariants();
    return TestFailed();
  } catch(BadConversionException&) {}
      
  try {
    variants_values.ToInteger();
    return TestFailed();
  } catch(BadConversionException&) {}
  
  try {
    variants_values.ToDecimal();
    return TestFailed();
  } catch(BadConversionException&) {}
  
  try {
    variants_values.ToString();
    return TestFailed();
  } catch(BadConversionException&) {}
   
  try {
    variants_values.ToVariants();
  } catch(BadConversionException&) {
    return TestFailed();
  }
  
  Log::Print(LogLevel::Info, "TEST PASSED");
  
  return 0;
}
