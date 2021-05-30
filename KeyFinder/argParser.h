#include <stdio.h>
#include <string>
#include <sstream>
#include <vector>

#include "util.h"
#include "secp256k1.h"
#include "KeySearchTypes.h"

bool parseKeyspace(const std::string &s, secp256k1::uint256 &start, secp256k1::uint256 &end);
bool parseUIntList(const std::string &s, std::vector<unsigned int> &v, char delimiter);
std::string formatUIntList(const std::vector<unsigned int> &v, char delimiter);
bool parseList(const std::string &s, secp256k1::uint256 &start, secp256k1::uint256 &end);
int parseCompressionString(const std::string &s);
std::string getCompressionString(int mode);
bool parseShare(const std::string &s, uint32_t &idx, uint32_t &total);
