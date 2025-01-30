#include <cfgtk/lexer.hpp>
#include <filesystem>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;
using namespace cfg;

int main(int argc, char **argv) {
  fs::current_path(fs::absolute(fs::path{argv[0]}.parent_path()));
  if (argc < 3) {
    std::cerr << "Too few paramaters; Usage: "
                 "<token-desc-file> <expected output> <input>\n";
    return 1;
  }

  lexer_table_t tbl{};
  const std::string data{argv[1]};
  if (auto st = read_from_file(data, &tbl); st != result::success) {
    switch (st) {
    case result::file_access_failure:
      std::cerr << "Failed to open data file: '" << data << "'" << std::endl;
      return 2;
    case result::format_error:
      std::cerr << "The data file '" << data << "' is not valid.\n";
      return 3;
    default:
      std::cerr << "The data file '" << data << "'"
                << " contains a bad token type.\n";
      return 4;
    }
  }

  auto cont = std::vector<std::string>{};
  if (argc > 3)
    cont = flt::to_container<std::vector>(argc, argv, 3);

  const std::string expected{argv[2]};
  std::stringstream summary{};

  auto tokens = tokenize(&tbl, &cont);

  for (std::size_t i = 0; i < tokens.size(); ++i)
    summary << tokens[i].id + ":" << tokens[i].value
            << (i == tokens.size() - 1 ? "" : " ");

  if (expected != summary.str()) {
    std::cerr << "Expected: " << expected << std::endl;
    std::cout << "But have: " << summary.str() << std::endl;
    std::cout << "The token table is:\n" << to_string(&tbl) << std::endl;
    return 5;
  }
}
