#include "process/variant.h"

#include <sstream>

#include "process/exception.h"

namespace cwa {
namespace process {

class Variant::Private {
public:
  Type type = Type::None;
  int integer_value = 0;
  double decimal_value = 0;
  std::string string_value;
  Variants variants;
};

Variant::Variant() : data_(new Private) {
  data_->type = Type::None;
}

Variant::Variant(const Variant & value) {
  // Copy internal content
  data_.reset(new Private(*value.data_));
}

Variant & Variant::operator=(const Variant & value) {
  // Copy internal content
  data_.reset(new Private(*value.data_));
  return *this;
}

Variant::Variant(int value) : data_(new Private) {
  data_->type = Type::Integer;
  data_->integer_value = value;
}

Variant::Variant(double value) : data_(new Private) {
  data_->type = Type::Decimal;
  data_->decimal_value = value;
}

Variant::Variant(const std::string & value) : data_(new Private) {
  data_->type = Type::String;
  data_->string_value = value;
}

Variant::Variant(const char* value) : data_(new Private) {
  data_->type = Type::String;
  data_->string_value = value;
}

Variant::Variant(const Variants & values) : data_(new Private) {
  data_->type = Type::Variants;
  data_->variants = values;
}

Variant::Type Variant::GetType() const {
  return data_->type;
}

int Variant::ToInteger() const {
  if(data_->type != Type::Integer) {
    throw BadConversionException();
  }
  return data_->integer_value;
}

double Variant::ToDecimal() const {
  if(data_->type != Type::Decimal) {
    throw BadConversionException();
  }
  return data_->decimal_value;
}

const std::string & Variant::ToString() const {
  if(data_->type != Type::String) {
    throw BadConversionException();
  }
  return data_->string_value;
}

const Variants & Variant::ToVariants() const {
  if(data_->type != Type::Variants) {
    throw BadConversionException();
  }
  return data_->variants;
}

std::string Variant::SerializeValues(const VariantMap & values, const std::string & prefix) {
  std::stringstream result;
  for(const auto & entry : values) {
    if(entry.first != values.begin()->first) {
      result << " ";
    }
    result << prefix << entry.first << " " << entry.second;
  }
  return result.str();
  
}

std::ostream& operator<<(std::ostream& os, const Variant& value)
{
  switch(value.GetType()) {
    case Variant::Type::None:
      os << "!!! Type::None !!!";
      break;
    case Variant::Type::Integer:
      os << value.ToInteger();
      break;
    case Variant::Type::Decimal:
      os << value.ToDecimal();
      break;
    case Variant::Type::String:
      os << value.ToString();
      break;
    case Variant::Type::Variants:
      if(value.ToVariants().empty()) break;
      
      os << value.ToVariants()[0];
      for(int i=1;i<value.ToVariants().size();i++) {
        const Variant & variant = value.ToVariants()[i];
        os << "," << variant;
      }
      break;
  }
  
  return os;
}

bool Variant::operator==(const Variant & value) const {
  
  // If not same type, not equal
  if(data_->type != value.data_->type) return false;
  
  switch (data_->type) {
    case Type::None:
      return false;
    case Type::Integer:
      return data_->integer_value == value.data_->integer_value;
    case Type::Decimal:
      return data_->decimal_value == value.data_->decimal_value;
    case Type::String:
      return data_->string_value == value.data_->string_value;
    case Type::Variants:
      return data_->variants == value.data_->variants;
    default:
      break;
  }
  
  return false;
}

bool operator==(const Variants & lvalue, const Variants & rvalue) {
  
  if(lvalue.size() != rvalue.size()) return false;
  
  bool ok = true;
  for(size_t i=0;i<lvalue.size();i++) {
    ok &= lvalue[i] == rvalue[i];
  }
  
  return ok;
}

bool operator!=(const Variants & lvalue, const Variants & rvalue) {
  return !(lvalue == rvalue);
}

}
}
