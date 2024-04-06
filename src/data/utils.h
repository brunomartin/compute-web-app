#ifndef CWA_DATA_UTILS_H
#define CWA_DATA_UTILS_H

#include <string>
#include <sstream>
#include <vector>
#include <iterator>
#include <chrono>

using std::chrono::high_resolution_clock;
using std::chrono::microseconds;
using std::chrono::duration_cast;

template<char delimiter>
class WordDelimitedBy : public std::string {};

template<char delimiter>
std::istream& operator>>(std::istream& is, WordDelimitedBy<delimiter>& output) {
   std::getline(is, output, delimiter);
   return is;
}
  
template<char delimiter>
std::vector<std::string> Split(const std::string & string) {
  std::istringstream iss(string);
  std::istream_iterator<WordDelimitedBy<delimiter>> begin(iss);
  std::istream_iterator<WordDelimitedBy<delimiter>> end;
  std::vector<std::string> results(begin, end);
  return results;
}

class Duration {
public:
  Duration() {
    start_ = high_resolution_clock::now();
  }
  
  void Start() {
    start_ = high_resolution_clock::now();    
  }
  
  int GetElapsedUs() {
    auto duration = high_resolution_clock::now() - start_;
    return duration_cast<microseconds>(duration).count();
  }
  
private:
  high_resolution_clock::time_point start_;
};

#endif // CWA_DATA_UTILS_H
