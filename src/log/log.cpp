#include "log.h"

#include <map>
#include <fstream>

#include <log/plog/Log.h>
#include <log/plog/Initializers/RollingFileInitializer.h>
#include <log/plog/Formatters/CwaTxtFormatter.h>
#include <log/plog/Appenders/CwaConsoleAppender.h>

#include <log/plog/Formatters/TxtFormatter.h>
#include <log/plog/Appenders/RollingFileAppender.h>

namespace cwa {

class Log::Private {
public:
  std::map<LogLevel, plog::Severity> level_map = {
    {LogLevel::Trace, plog::verbose},
    {LogLevel::Debug, plog::debug},
    {LogLevel::Info, plog::info},
    {LogLevel::Error, plog::error},
    {LogLevel::Fatal, plog::fatal},
  };
};

std::unique_ptr<Log::Private> Log::data_(new Log::Private());

void Log::Init(int argc, char* argv[]) {
  LogLevel level = LogLevel::Info;
  std::string filename = "";
  
  for(int i=1;i<argc;i++) {
    if(strcmp(argv[i], "-vv") == 0) {
      level = LogLevel::Trace;
    }
  
    if(strcmp(argv[i], "-v") == 0) {
      level = LogLevel::Debug;
    }
    
    if(strcmp(argv[i], "-q") == 0) {
      level = LogLevel::Fatal;
    }
  
    if(strcmp(argv[i], "--log-file") == 0) {
      if(argc > i+1) {
        filename.assign(argv[i+1]);
      }
    }
  }
  
  Log::Init(level, filename);
}

void Log::Init(LogLevel level, const std::string & filename) {
  static plog::SimpleConsoleAppender<plog::SimpleTxtFormatter> console_appender;
  plog::init<1>(data_->level_map[level], &console_appender);
  
  if(!filename.empty()) {
    // Trunc the log file, any previous log record are deleted
    std::ofstream file(filename, std::ios::trunc);
    file.close();
    static plog::RollingFileAppender<plog::SimpleTxtFormatter> file_appender(filename.c_str());
    plog::init<2>(data_->level_map[LogLevel::Trace], &file_appender);
  }
}

void Log::Print(LogLevel level, const char* format, ...) {
    
  char* str = NULL;
  va_list ap;

  va_start(ap, format);
  int len = vasprintf(&str, format, ap);
  static_cast<void>(len);
  va_end(ap);

  PLOG_(1, data_->level_map[level]).printf(str);
  PLOG_(2, data_->level_map[level]).printf(str);
}

}
