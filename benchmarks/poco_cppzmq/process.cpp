#include "process.h"

#include <cmath>

void processDataset(const void* raw_data_in, size_t raw_size_in, void* raw_data_out, size_t raw_size_out) {

//  return;
  
  const uint16_t* data_in = (const uint16_t*)raw_data_in;
  uint16_t* data_out = (uint16_t*)raw_data_out;
  
  size_t size_in = raw_size_in/sizeof(uint16_t);
    
  for(int i=0;i<size_in;i++) {
    uint16_t value = data_in[i];
    
    for(int j=0;j<10;j++) {
      value *= 1.5;
      value /= 2.;
    }
    
    data_out[i] = value;
  }
  
}
