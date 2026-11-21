#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <regex>
#include <string>

#include "hasher.h"
#include "lazy_file.h"
#include "pool_allocator.h"
#include "seq.h"

class FileDuplicateFinder
{
public:
    using FileGroup = MyVector<FileReader>;
    using SizeGroupsMap = MyUnorderedMap<std::uintmax_t, FileGroup>;

    struct Config
    {
        MyVector<std::filesystem::path> include_dirs;
        MyVector<std::filesystem::path> exclude_dirs;
        int max_depth = 0;
        std::uintmax_t min_size = 2;
        MyVector<std::string> raw_masks;
        std::size_t block_size = 1024;
        std::string hash_algo = "md5";
    };

    explicit FileDuplicateFinder(std::unique_ptr<Config> cfg);
    void start();

private:
    static std::regex mask_to_regex(const std::string& mask);
    bool matches_masks(const std::string& filename) const;

    void scan_directory(
        const std::filesystem::path& root,
        SizeGroupsMap& size_groups,
        MyUnorderedSet<std::string>& seen_files
    );

    MyVector<MyVector<std::filesystem::path>>
    find_duplicates_in_group(MyVector<FileReader>& files);

    std::unique_ptr<Config> cfg_;
    hasher::hash_function hasher_;
    MyVector<std::regex> masks_;
    MyUnorderedSet<std::string> excluded_canonical_;
};