#ifndef CWA_LOG_LOG_H
#define CWA_LOG_LOG_H

#include <string>
#include <memory>

namespace cwa {

enum class LogLevel {
  Trace,
  Debug,
  Info,
  Error,
  Fatal
};

class Log {
public:

  static void Init(int argc, char* argv[]);
  static void Init(LogLevel level, const std::string & filename = "");
  static void Print(LogLevel level, const char* format, ...);
  
private:
  class Private;
  static std::unique_ptr<Private> data_;
};

}

#endif // CWA_LOG_LOG_H
