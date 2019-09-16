#include <CacheSet.hpp>
#include <MOSICache.hpp>
#include <iostream>

class Bus;

MOSICache::MOSICache(_id id, Bus* pbus, Simulator::Statistics& stats)
		: Cache(id, pbus, stats) {

}


