#pragma once

#include <cassert>
#include <string>
#include <vector>

#include <boost/any.hpp>
#include <boost/program_options.hpp>

#include "seq.h"

template <typename T, typename Alloc>
void validate(
    boost::any& v,
    const std::vector<std::string>& xs,
    Seq<T, Alloc>*,
    int
)
{
    namespace po = boost::program_options;

    if (v.empty())
    {
        v = boost::any(Seq<T, Alloc>());
    }

    auto* tv = boost::any_cast<Seq<T, Alloc>>(&v);
    assert(tv != nullptr);

    for (const auto& s : xs)
    {
        boost::any a;
        std::vector<std::string> cv{s};
        po::validate(a, cv, static_cast<T*>(nullptr), 0);
        tv->push_back(boost::any_cast<T>(a));
    }
}