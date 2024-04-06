#ifndef CWA_PROCESS_VARIANT_H
#define CWA_PROCESS_VARIANT_H

#include <string>
#include <cstdint>
#include <memory>
#include <vector>
#include <map>
#include <iostream>

namespace cwa {
namespace process {

class Variant;
typedef std::vector<Variant> Variants;
typedef std::map<std::string, Variant> VariantMap;

class Variant {
public:
  
  enum class Type {
    None,
    Integer,
    Decimal,
    String,
    Variants
  };
  
  Variant();
  Variant(const Variant & value);
  Variant & operator=(const Variant & value);
  
  Variant(int value);
  Variant(double value);
  Variant(const std::string & value);
  Variant(const char* value);
  Variant(const Variants & values);
    
  Type GetType() const;
  
  int ToInteger() const;
  double ToDecimal() const;
  const std::string & ToString() const;
  const Variants & ToVariants() const;
  
  bool operator==(const Variant & value) const;
  
  static std::string SerializeValues(const VariantMap & values, const std::string & prefix = "");
  
private:
  class Private;
  std::shared_ptr<Private> data_;
};

std::ostream& operator<<(std::ostream& os, const Variant& value);
bool operator==(const Variants & lvalue, const Variants & rvalue);
bool operator!=(const Variants & lvalue, const Variants & rvalue);

}
}

#endif // CWA_PROCESS_VARIANT_H
