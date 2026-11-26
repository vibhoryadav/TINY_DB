#include <iostream>
#include <string_view>

#include "tinydb/build_info.hpp"

int main(int argc, char* argv[]) {
  if (argc > 1 && std::string_view(argv[1]) == "--version") {
    std::cout << tinydb::GetVersionBanner() << '\n';
    return 0;
  }

  std::cout << tinydb::GetVersionBanner() << '\n';
  std::cout
      << "Bootstrap executable ready. Follow docs/roadmap/NEXT_SESSION.md to begin DAY-001.\n";
  return 0;
}
