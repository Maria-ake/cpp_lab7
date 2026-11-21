#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "duplicate_finder.h"
#include "validate.h"

namespace po = boost::program_options;

int main(int argc, char* argv[])
{
    try
    {
        auto cfg = std::make_unique<FileDuplicateFinder::Config>();
        MyVector<std::string> include_dirs;
        MyVector<std::string> exclude_dirs;
        std::uintmax_t min_size_exclusive = 1;

        po::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "Show help message")
            ("include,i", po::value<MyVector<std::string>>(&include_dirs)->multitoken()->required(), "Directories to scan")
            ("exclude,e", po::value<MyVector<std::string>>(&exclude_dirs)->multitoken(), "Directories to exclude")
            ("depth,d", po::value<int>(&(cfg->max_depth))->default_value(0), "Scan depth (0 = current directory)")
            ("min-size,m", po::value<std::uintmax_t>(&min_size_exclusive)->default_value(1), "Minimum file size in bytes")
            ("mask,f", po::value<MyVector<std::string>>(&(cfg->raw_masks))->multitoken(), "Case-insensitive file masks (*.txt file_?.log)")
            ("block-size,s", po::value<std::size_t>(&(cfg->block_size))->default_value(1024), "Block size S in bytes")
            ("hash,H", po::value<std::string>(&(cfg->hash_algo))->default_value("md5"), "Hash algorithm (md5, crc32)");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help"))
        {
            std::cout << desc << std::endl;
            return EXIT_SUCCESS;
        }

        po::notify(vm);

        if (cfg->block_size == 0)
        {
            std::cerr << "Error: block-size must be > 0" << std::endl;
            return EXIT_FAILURE;
        }

        for (const auto& d : include_dirs)
        {
            cfg->include_dirs.emplace_back(d);
        }

        for (const auto& d : exclude_dirs)
        {
            cfg->exclude_dirs.emplace_back(d);
        }

        cfg->min_size = min_size_exclusive + 1;

        FileDuplicateFinder finder(std::move(cfg));
        finder.start();
    }
    catch (const po::error& ex)
    {
        std::cerr << "Error: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Error: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}