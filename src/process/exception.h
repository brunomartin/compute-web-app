#ifndef CWA_PROCESS_EXCEPTION_H
#define CWA_PROCESS_EXCEPTION_H

#include <exception>

namespace cwa {
namespace process {

class Exception : public std::exception {
public:
private:
};

class BadConversionException : public Exception {
public:
private:
};

}
}

#endif // CWA_DATA_EXCEPTION_H
