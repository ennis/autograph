#ifndef ERROR_HPP
#define ERROR_HPP

#include <string>
#include <stdexcept>
#include <iostream>

namespace ag
{
	void failWith(std::string message) {
		std::cerr 
			<< "\n\n===============================================================\n"
			<< "Error: " << message << "\n";
		throw std::runtime_error(message.c_str());
	}
}

#endif // !ERROR_HPP
