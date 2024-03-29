# Cache Coherence Simulator
Simulator that simulates multiprocessor caches and involved cache coherence protocols - MSI, MESI, MOESI.

## Structure

```
├── include/ - Contains header files
│   ├── Bus.hpp
│   ├── Cache.hpp
│   ├── CacheSet.hpp
│   ├── Constants.hpp
│   ├── MICache.hpp
│   ├── MSICache.hpp
│   ├── MESICache.hpp
│   ├── MOSICache.hpp
│   ├── MOESICache.hpp
│   └── Simulator.hpp
├── input/ - Contains Input files (trace files)
├── makefile
├── README.txt
└── src
    ├── Bus.cpp - Represents the communication bus
    ├── Cache.cpp - Represents a collection of CacheSet(s)
    ├── CacheSet.cpp - Represent a Set in the Cache
    ├── main.cpp - Main application driver
    ├── MICache.hpp - Cache that adheres to MI protocol
    ├── MSICache.hpp - Cache that adheres to MSI protocol
    ├── MESICache.cpp - Cache that adheres to MESI protocol
    ├── MOSICache.cpp - Cache that adheres to MOSI protocol
    ├── MOESICache.cpp - Cache that adheres to MOESI protocol
    └── Simulator.cpp - Maintains Cache and Bus Instances, simulates by 
                        forwarding requests read from file
```



## Building & Running
| Task     | Command                           |
| -------- | --------------------------------- |
| Building | `make all`                        |
| Cleaning | `make clean`                      |
| Running  | `./bin/main.o <input_trace_file>` |

**Note:** To choose protocol edit line #25 in `include/Constants.hpp` to `MI`, `MSI`, `MOSI`, `MESI` or `MOESI`:

```

const protocol PROTOCOL = MOESI;
	
```

