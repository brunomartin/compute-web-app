#pragma once
#include <plog/Record.h>
#include <plog/Util.h>
#include <iomanip>
#include <ctime>

#include <plog/Formatters/TxtFormatter.h>

namespace plog
{

/* Subtract the ‘struct timeval’ values X and Y,
   storing the result in RESULT.
   Return 1 if the difference is negative, otherwise 0. */
int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y)
{
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

class SimpleTxtFormatter : public TxtFormatterImpl<false>
{
public:

  static util::nstring format(const Record& record)
  {
    util::Time record_time = record.getTime();
    
    if(record_time.millitm < start_time.millitm) {
      record_time.millitm += 1000;
      record_time.time -= 1;
    }
    record_time.time -= start_time.time;
    record_time.millitm -= start_time.millitm;
    
    util::nostringstream ss;
#if 1
    ss << std::setfill(PLOG_NSTR('0')) << std::setw(6) << record_time.time << PLOG_NSTR(".") << std::setfill(PLOG_NSTR('0')) << std::setw(3) << static_cast<int> (record_time.millitm) << PLOG_NSTR(" ");
#else
    tm t;
    util::gmtime_s(&t, &record_time.time);

    ss << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_hour << PLOG_NSTR(":") << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_min << PLOG_NSTR(":") << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_sec << PLOG_NSTR(".") << std::setfill(PLOG_NSTR('0')) << std::setw(3) << static_cast<int> (record.getTime().millitm) << PLOG_NSTR(" ");
#endif
    ss << std::setfill(PLOG_NSTR(' ')) << std::setw(5) << std::left << severityToString(record.getSeverity()) << PLOG_NSTR(" ");
    ss << record.getMessage() << PLOG_NSTR("\n");

    return ss.str();
  }
  
  static util::Time start_time;
  
private:
};

util::Time SimpleTxtFormatter::start_time;
}
