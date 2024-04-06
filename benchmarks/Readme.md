Benchmarks for components
---

- cpp-httplib
This folder contains server and client application to test data grabbing and sending from running process. with a 10MB file, ~8ms to read, ~80ms to read and retrieve via http : 10x higher !!!
The conclusion is that it is far more efficient to read file directly than via a http requests

- cppzmq
This folder contains sources with same purpose. with a 10MB file, ~8ms to read, ~12ms to read and retrieve via zmq : great !

- poco_cppzmq
This folder contains server/client apps for processing bulk data for benchmarking purpose using poco for launching client processes and zmq to make data available to clients
  - only load data and write it back in files,
    - on MB Pro i5 2.3GHz:
      - 100MB = 100x1024kB datasets, computing alone, no process : ~300ms
      - 100MB = 100x1024kB datasets, 4 workers, no process : ~350ms
    - on rpi3 Cortex-A53 1.4GHz:
      - 100MB = 100x1024kB datasets, computing alone, no process : ~300ms
      - 100MB = 100x1024kB datasets, 4 workers, no process : ~350ms
  - simple process on uint16 data with double sums and multiply,
    - on MB Pro i5 2.3GHz:
      - 100MB = 100x1024kB datasets, computing alone, simple process : ~3.1s
      - 100MB = 100x1024kB datasets, 2 workers, simple process : ~1.8s
      - 100MB = 100x1024kB datasets, 4 workers, simple process : ~1.4s
      - 1GB = 1000x1024kB datasets, computing alone, simple process : ~31s
      - 1GB = 1000x1024kB datasets, 2 workers, simple process : ~22s
      - 1GB = 1000x1024kB datasets, 4 workers, simple process : ~15s
    - on rpi3 Cortex-A53 1.4GHz:
      - 100MB = 100x1024kB datasets, computing alone, simple process : ~6.5s
      - 100MB = 100x1024kB datasets, 2 workers, simple process : ~1.7s
      - 100MB = 100x1024kB datasets, 4 workers, simple process : ~1.5s

- poco_cppzmq_2
Same as previous but with dedicated thread for pulling and pushing data in server app.
When data is push, program does not wait to push another data.
Synchonize it so that it prepare to push and push when worker is ready to pull it.

