#ifndef CWA_DATA_EXCEPTION_H
#define CWA_DATA_EXCEPTION_H

#include <exception>

namespace cwa {
namespace data {

class Exception : public std::exception {
public:
  Exception() {}
  Exception(const char * message) : message_(message) {}
  
private:
  std::string message_;
};

class OrphanDatasetException : public Exception {
public:
private:
};

class NoMoreDatasetException : public Exception {
public:
private:
};

class NoMoreSupplierException : public Exception {
public:
private:
};

class CannotConnectToServerException : public Exception {
public:
private:
};

class TimeoutException : public Exception {
public:
private:
};

}
}

#endif // CWA_DATA_EXCEPTION_H
