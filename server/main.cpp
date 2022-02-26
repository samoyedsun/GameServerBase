// mersenne_twister_engine::seed example
#include <iostream>
#include "wrandom.h"
#include "boost/version.hpp"

namespace wang
{
	void dump()
	{
		for (int i = 0; i < 10; ++i)
			std::cout << s_rand.rand(1, 100) << "\t" << s_rand.rand(3) << "\t" << s_rand.randf() << std::endl;
	}
}

int main()
{
	// dependent Boost1.60
	std::cout << "Boost version: "
		<< BOOST_VERSION / 100000
		<< "."
		<< BOOST_VERSION / 100 % 1000
		<< "."
		<< BOOST_VERSION % 100
		<< std::endl;

	wang::dump();
	system("pause");

	return 0;
}