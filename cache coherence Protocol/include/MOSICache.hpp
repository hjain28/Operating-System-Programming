#ifndef INCLUDE_MOSICACHE_HPP_
#define INCLUDE_MOSICACHE_HPP_

#include "Bus.hpp"
#include "Cache.hpp"
#include "Constants.hpp"
#include "Simulator.hpp"

class MOSICache: public Cache {
	private:
		void read_request(_address);
		void write_request(_address);

	public:
		MOSICache(_id, Bus*, Simulator::Statistics&);

		virtual bool handle_bus_request(Bus::BusRequest);
};

#endif /* INCLUDE_MOSICACHE_HPP_ */
