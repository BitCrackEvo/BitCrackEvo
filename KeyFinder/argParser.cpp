#include "argParser.h"


/**
 * Parses the start:end key pair. Possible values are:
 start
 start:end
 start:+offset
 :end
 :+offset
 */
bool parseKeyspace(const std::string &s, secp256k1::uint256 &start, secp256k1::uint256 &end)
{
    size_t pos = s.find(':');

    if(pos == std::string::npos) {
        start = secp256k1::uint256(s);
        end = secp256k1::N - 1;
    } else {
        std::string left = s.substr(0, pos);

        if(left.length() == 0) {
            start = secp256k1::uint256(1);
        } else {
            start = secp256k1::uint256(left);
        }

        std::string right = s.substr(pos + 1);

        if(right[0] == '+') {
            end = start + secp256k1::uint256(right.substr(1));
        } else {
            end = secp256k1::uint256(right);
        }
    }

    return true;
}

bool parseDevices(const std::string &s, secp256k1::uint256 &start, secp256k1::uint256 &end)
{
    std::vector<std::string> v;
 
    std::stringstream ss(s);
 
    while (ss.good()) {
        std::string substr;
        std::getline(ss, substr, ',');
        v.push_back(substr);
    }
 
    /*for (size_t i = 0; i < v.size(); i++)
        std::cout << v[i] << endl;*/

    return true;
}

int parseCompressionString(const std::string &s)
{
    std::string comp = util::toLower(s);

    if(comp == "both") {
        return PointCompressionType::BOTH;
    }

    if(comp == "compressed") {
        return PointCompressionType::COMPRESSED;
    }

    if(comp == "uncompressed") {
        return PointCompressionType::UNCOMPRESSED;
    }

    throw std::string("Invalid compression format: '" + s + "'");
}

static std::string getCompressionString(int mode)
{
    switch(mode) {
    case PointCompressionType::BOTH:
        return "both";
    case PointCompressionType::UNCOMPRESSED:
        return "uncompressed";
    case PointCompressionType::COMPRESSED:
        return "compressed";
    }

    throw std::string("Invalid compression setting '" + util::format(mode) + "'");
}

/**
 * Parses a string in the form of x/y
 */
bool parseShare(const std::string &s, uint32_t &idx, uint32_t &total)
{
    size_t pos = s.find('/');
    if(pos == std::string::npos) {
        return false;
    }

    try {
        idx = util::parseUInt32(s.substr(0, pos));
    } catch(...) {
        return false;
    }

    try {
        total = util::parseUInt32(s.substr(pos + 1));
    } catch(...) {
        return false;
    }

    if(idx == 0 || total == 0) {
        return false;
    }

    if(idx > total) {
        return false;
    }

    return true;
}
