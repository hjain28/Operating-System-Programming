#ifndef INCLUDE_MICACHE_HPP_
#define INCLUDE_MICACHE_HPP_

#include "Bus.hpp"
#include "Cache.hpp"
#include "Constants.hpp"
#include "Simulator.hpp"

class MICache: public Cache {
	private:
		void read_request(_address);
		void write_request(_address);

	public:
		MICache(_id, Bus*, Simulator::Statistics&);

		virtual bool handle_bus_request(Bus::BusRequest);
};
#endif /*INCLUDE_MICACHE_HPP_ */
