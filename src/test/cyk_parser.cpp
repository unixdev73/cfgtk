#include <cfgtk/lexer.hpp>
#include <cfgtk/parser.hpp>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

int main(int argc, char **argv) {
  fs::current_path(fs::absolute(fs::path{argv[0]}.parent_path()));
  if (argc < 4) {
    std::cerr << "Too few parameters; Usage: "
                 "<grammar-file> <token-table-file> <input-sequence>\n";
    return 1;
  }

  cfg::grammar_t ig{};
  if (cfg::read_from_file(argv[1], &ig) != cfg::result::success) {
    std::cerr << "Reading grammar at: '" << argv[1] << "' failed.\n";
    return 2;
  }

  cfg::lexer_table_t tbl{};
  const std::string data{argv[2]};
  if (auto st = cfg::read_from_file(data, &tbl); st != cfg::result::success) {
    switch (st) {
    case cfg::result::file_access_failure:
      std::cerr << "Failed to open data file: '" << data << "'" << std::endl;
      return 3;
    case cfg::result::format_error:
      std::cerr << "The data file '" << data << "' is not valid.\n";
      return 3;
    default:
      std::cerr << "The data file '" << data << "'"
                << " contains a bad token type.\n";
      return 3;
    }
  }

  const auto input = flt::to_container<std::vector>(argc, argv, 3);
  const auto tokens = cfg::tokenize(&tbl, &input);

  cfg::grammar_t g{};
  cfg::cnf_info conf{};
  conf.filter = cfg::cnf_filter::unique0 | cfg::cnf_filter::start |
                cfg::cnf_filter::term | cfg::cnf_filter::bin |
                cfg::cnf_filter::del | cfg::cnf_filter::unique1 |
                cfg::cnf_filter::unit | cfg::cnf_filter::unique2 |
                cfg::cnf_filter::group | cfg::cnf_filter::prune;

  if (cfg::to_cnf(&ig, &g, &conf) != cfg::result::success) {
    std::cerr << "Converting grammar to CNF failed." << std::endl;
    return 4;
  }

  cfg::text_encoding e{};
  std::cout << "CFG GRAMMAR:\n" << cfg::to_string(&ig, &e) << "\n\n";
  std::cout << "CNF GRAMMAR:\n" << cfg::to_string(&g, &e) << "\n\n";

  const auto ss = cfg::get_start(&g);
  const auto ch = cfg::cyk(&g, &tokens);
  const bool ok = cfg::is_valid(&ch, ss);

  std::cout << "parsing complete; result: " << (ok ? "OK" : "NOK") << "\n\n";
  if (tokens.size() <= 7)
    std::cout << cfg::to_string(&ch) << std::endl;

  if (ok) {
    if (tokens.size() <= 7)
      std::cout << std::endl;
    auto tree = cfg::get_trees(&ch, ss).front();
    std::cout << cfg::to_string(&tree, &tokens) << std::endl;
  }
  return !ok;
}
