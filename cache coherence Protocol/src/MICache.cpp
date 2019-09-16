#include <CacheSet.hpp>
#include <MICache.hpp>
#include <iostream>

class Bus;

MICache::MICache(_id id, Bus* pbus, Simulator::Statistics& stats)
		: Cache(id, pbus, stats) {

}

