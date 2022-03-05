// mersenne_twister_engine::seed example
#include <iostream>
#include "wrandom.h"
#include "wcmd_param.h"
#include "boost/version.hpp"

namespace wang
{
	void dump()
	{
		for (int i = 0; i < 10; ++i)
			std::cout << s_rand.rand(1, 100) << "\t" << s_rand.rand(3) << "\t" << s_rand.randf() << std::endl;

		std::string cmd = "help 1 2";
		wcmd_param param;
		param.parse_cmd(cmd);

		int num1;
		param.get_next_param(num1);
		std::cout << num1 << std::endl;
		param.get_next_param(num1);
		std::cout << num1 << std::endl;
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