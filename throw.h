#pragma once

#include <sstream>
#include <stdexcept>

template<typename E = std::runtime_error>
class Throw {
	std::stringstream ss;

public:
	template<typename T>
	std::ostream& operator << (const T &t) {
		return ss << t;
	}
	~Throw() noexcept(false) {
		throw E(ss.str());
	}
};
