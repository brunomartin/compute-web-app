#pragma once
#include <plog/Appenders/ConsoleAppender.h>

namespace plog {

template<class Formatter>
class SimpleConsoleAppender : public ConsoleAppender<Formatter>
{
public:
  SimpleConsoleAppender(OutputStream outStream = streamStdOut) : ConsoleAppender<Formatter>(outStream) {
    util::ftime(&Formatter::start_time);
  }

  virtual void write(const Record& record)
  {
    ConsoleAppender<Formatter>::write(record);
  }

private:

protected:
  
};
}
