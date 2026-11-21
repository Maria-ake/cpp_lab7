#pragma once

#include <cstddef>
#include <functional>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <string>

#include <boost/algorithm/hex.hpp>
#include <boost/crc.hpp>
#include <boost/uuid/detail/md5.hpp>

namespace hasher
{
    using hash_function = std::function<std::string(const char*, std::size_t)>;

    inline std::string md5hash(const char* data, std::size_t size)
    {
        boost::uuids::detail::md5 hash;
        boost::uuids::detail::md5::digest_type digest;
        hash.process_bytes(data, size);
        hash.get_digest(digest);

        std::string res;
        res.reserve(32);
        boost::algorithm::hex_lower(digest, std::back_inserter(res));
        return res;
    }

    inline std::string crc32hash(const char* data, std::size_t size)
    {
        boost::crc_32_type crc;
        crc.process_bytes(data, size);

        std::ostringstream oss;
        oss << std::hex << std::setw(8) << std::setfill('0') << crc.checksum();
        return oss.str();
    }
}