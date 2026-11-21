#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <system_error>
#include <utility>

#include "duplicate_finder.h"

namespace fs = std::filesystem;

FileDuplicateFinder::FileDuplicateFinder(std::unique_ptr<Config> cfg)
    : cfg_(std::move(cfg))
{
    if (cfg_->hash_algo == "md5")
    {
        hasher_ = hasher::md5hash;
    }
    else if (cfg_->hash_algo == "crc32")
    {
        hasher_ = hasher::crc32hash;
    }
    else
    {
        throw std::invalid_argument("Unsupported hash algorithm: " + cfg_->hash_algo);
    }

    masks_.reserve(cfg_->raw_masks.size());
    for (const auto& m : cfg_->raw_masks)
    {
        masks_.push_back(mask_to_regex(m));
    }

    std::error_code ec;
    for (const auto& ex : cfg_->exclude_dirs)
    {
        auto canon = fs::canonical(ex, ec);
        if (!ec)
        {
            excluded_canonical_.insert(canon.string());
        }
        ec.clear();
    }
}

void FileDuplicateFinder::start()
{
    SizeGroupsMap size_groups;
    MyUnorderedSet<std::string> seen_files;

    for (const auto& dir : cfg_->include_dirs)
    {
        scan_directory(dir, size_groups, seen_files);
    }

    bool first_group_printed = false;

    for (auto& entry : size_groups)
    {
        auto& files = entry.second;
        if (files.size() < 2)
        {
            continue;
        }

        const auto duplicate_clusters = find_duplicates_in_group(files);
        for (const auto& cluster : duplicate_clusters)
        {
            if (cluster.size() < 2)
            {
                continue;
            }

            if (first_group_printed)
            {
                std::cout << std::endl;
            }

            for (const auto& path : cluster)
            {
                std::cout << path.string() << std::endl;
            }
            first_group_printed = true;
        }
    }
}

std::regex FileDuplicateFinder::mask_to_regex(const std::string& mask)
{
    std::string rx = "^";
    for (char c : mask)
    {
        switch (c)
        {
            case '*':
                rx += ".*";
                break;
            case '?':
                rx += ".";
                break;
            case '.': case '+': case '(': case ')':
            case '[': case ']': case '{': case '}':
            case '^': case '$': case '|': case '\\':
                rx += '\\';
                rx += c;
                break;
            default:
                rx += c;
                break;
        }
    }
    rx += "$";
    return std::regex(rx, std::regex::icase);
}

bool FileDuplicateFinder::matches_masks(const std::string& filename) const
{
    if (masks_.empty())
    {
        return true;
    }

    for (const auto& rx : masks_)
    {
        if (std::regex_match(filename, rx))
        {
            return true;
        }
    }
    return false;
}

void FileDuplicateFinder::scan_directory(
    const fs::path& root,
    SizeGroupsMap& size_groups,
    MyUnorderedSet<std::string>& seen_files
)
{
    std::error_code ec;
    auto canon_root = fs::canonical(root, ec);
    if (ec || !fs::exists(canon_root, ec) || !fs::is_directory(canon_root, ec))
    {
        return;
    }

    if (excluded_canonical_.count(canon_root.string()))
    {
        return;
    }

    fs::recursive_directory_iterator it(
        canon_root,
        fs::directory_options::skip_permission_denied,
        ec
    );
    fs::recursive_directory_iterator end;

    while (it != end)
    {
        const auto& entry = *it;
        auto canon_path = fs::canonical(entry.path(), ec);

        if (!ec && entry.is_directory(ec))
        {
            if (excluded_canonical_.count(canon_path.string()) || it.depth() >= cfg_->max_depth)
            {
                it.disable_recursion_pending();
            }
        }
        else if (!ec && entry.is_regular_file(ec))
        {
            std::uintmax_t fsize = entry.file_size(ec);
            if (!ec && fsize >= cfg_->min_size &&
                matches_masks(entry.path().filename().string()))
            {
                if (seen_files.insert(canon_path.string()).second)
                {
                    size_groups[fsize].emplace_back(
                        canon_path,
                        fsize,
                        cfg_->block_size,
                        hasher_
                    );
                }
            }
        }

        it.increment(ec);
        if (ec)
        {
            ec.clear();
        }
    }
}

MyVector<MyVector<fs::path>>
FileDuplicateFinder::find_duplicates_in_group(MyVector<FileReader>& files)
{
    MyVector<MyVector<fs::path>> result;
    if (files.size() < 2)
    {
        return result;
    }

    const std::size_t total_blocks = files[0].total_blocks();

    MyVector<MyVector<std::size_t>> buckets;
    buckets.emplace_back();
    buckets[0].reserve(files.size());
    for (std::size_t i = 0; i < files.size(); ++i)
    {
        buckets[0].push_back(i);
    }

    for (std::size_t block_idx = 0; block_idx < total_blocks && !buckets.empty(); ++block_idx)
    {
        MyVector<MyVector<std::size_t>> next_buckets;

        for (const auto& bucket : buckets)
        {
            MyUnorderedMap<std::string, MyVector<std::size_t>> sub_buckets;
            for (std::size_t file_idx : bucket)
            {
                const std::string& h = files[file_idx].get_block_hash(block_idx);
                sub_buckets[h].push_back(file_idx);
            }

            for (auto& entry : sub_buckets)
            {
                if (entry.second.size() >= 2)
                {
                    next_buckets.push_back(std::move(entry.second));
                }
            }
        }

        buckets = std::move(next_buckets);
    }

    for (const auto& bucket : buckets)
    {
        MyVector<fs::path> cluster;
        cluster.reserve(bucket.size());
        for (std::size_t file_idx : bucket)
        {
            cluster.push_back(files[file_idx].path());
        }
        result.push_back(std::move(cluster));
    }

    return result;
}