#include <FileWatch.hpp>
#include <iostream>
#include <string>
#include <iostream>

int main() {
      filewatch::FileWatch<std::string> watch {
            ".",
            [] (const std::string& path, const filewatch::Event event) {
                  std::cout << path << ' ' << filewatch::event_to_string(event) << '\n';
            }
      };

      while (true) {

      }
}
