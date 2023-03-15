#include <filesystem>
#include <FileWatch.hpp>
#include <iostream>
#include <string>
#include <iostream>

int main()
{
    filewatch::FileWatch<std::filesystem::path> watch(
        ".",
        [](const std::filesystem::path& path, const filewatch::Event change_type) {
        std::cout << path << "\n";
    }
    );

    while (true) {

    }
}