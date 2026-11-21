#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

#include "hasher.h"
#include "seq.h"

class FileReader
{
public:
    FileReader(
        std::filesystem::path path,
        std::uintmax_t file_size,
        std::size_t block_size,
        hasher::hash_function hasher
    )
        : path_(std::move(path))
        , file_size_(file_size)
        , block_size_(block_size)
        , total_blocks_((file_size_ + block_size_ - 1) / block_size_)
        , hasher_(std::move(hasher))
    {}

    const std::filesystem::path& path() const noexcept
    {
        return path_;
    }

    std::size_t total_blocks() const noexcept
    {
        return total_blocks_;
    }

    const std::string& get_block_hash(std::size_t block_index)
    {
        if (block_index < cached_hashes_.size())
        {
            return cached_hashes_[block_index];
        }

        if (!stream_.is_open())
        {
            stream_.open(path_, std::ios::binary);
        }

        MyVector<char> buffer(block_size_, '\0');
        if (stream_.good())
        {
            stream_.read(buffer.data(), static_cast<std::streamsize>(block_size_));
        }

        cached_hashes_.push_back(hasher_(buffer.data(), block_size_));

        if (cached_hashes_.size() == total_blocks_ && stream_.is_open())
        {
            stream_.close();
        }

        return cached_hashes_.back();
    }

    FileReader(const FileReader&) = delete;
    FileReader& operator=(const FileReader&) = delete;

    FileReader(FileReader&&) = default;
    FileReader& operator=(FileReader&&) = default;

private:
    std::filesystem::path path_;
    std::uintmax_t file_size_;
    std::size_t block_size_;
    std::size_t total_blocks_;
    hasher::hash_function hasher_;

    std::ifstream stream_;
    MyVector<std::string> cached_hashes_;
};