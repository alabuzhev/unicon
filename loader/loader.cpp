#include <iostream>
#include <stdexcept>

#include "../unicon/inject.hpp"

int main(int const Argc, char const* const Argv[])
{
	try
	{
		if (Argc > 2)
			throw std::runtime_error("Usage: unicon [conhost_pid]");

		inject(Argc == 2? std::atoi(Argv[1]) : 0);
		return 0;
	}
	catch (std::exception const& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
}
