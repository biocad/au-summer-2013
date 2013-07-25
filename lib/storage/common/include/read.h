#pragma once

#include <string>

class Read {
public:
	Read(const std::string & name, const std::string & seq, const std::string & qual) : name(name), seq(seq), qual(qual) {};
	Read(const std::string & name, const std::string & seq) : name(name), seq(seq){};
	Read(){};

	std::string name, seq, qual;
};