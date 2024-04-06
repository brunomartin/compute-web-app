#ifndef PROCESS_H
#define PROCESS_H

#include <vector>
#include <cstdint>

void processDataset(const void* raw_data_in, size_t raw_size_in, void* raw_data_out, size_t raw_size_out);

#endif // PROCESS_H
