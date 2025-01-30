#include <cfgtk/parser.hpp>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

int main(int argc, char **argv) {
  fs::current_path(fs::absolute(fs::path{argv[0]}.parent_path()));
  if (argc < 3) {
    std::cerr << "Too few parameters; Usage: "
                 "<expected-grammar-file> <input-grammar-file>\n";
    return 1;
  }

  cfg::grammar_t expd{}, input{};
  std::vector<cfg::grammar_t *> gseq{&expd, &input};
  for (std::size_t i = 0; i < gseq.size(); ++i) {
    const std::size_t index = i + 1;
    if (cfg::read_from_file(argv[index], gseq[i]) != cfg::result::success) {
      std::cerr << "Reading grammar[" << index << "]  at: '" << argv[index]
                << "' failed.\n";
      return 2;
    }
  }

  cfg::grammar_t gv{};
  cfg::cnf_info conf{};
  conf.filter = cfg::cnf_filter::unique0 | cfg::cnf_filter::start |
                cfg::cnf_filter::term | cfg::cnf_filter::bin |
                cfg::cnf_filter::del | cfg::cnf_filter::unique1 |
                cfg::cnf_filter::unit | cfg::cnf_filter::unique2 |
                cfg::cnf_filter::group | cfg::cnf_filter::prune;

  if (auto r = cfg::to_cnf(&input, &gv, &conf); r != cfg::result::success) {
    std::cerr << "Converting to CNF failed: ";
    switch (r) {
    case cfg::result::excessive_symbols:
      std::cerr << "More symbols than expected." << std::endl;
      break;
    default:
      std::cerr << "Unknown error." << std::endl;
      break;
    }
    return 3;
  }

  cfg::text_encoding e{};
  if (!is_equal(&gv, &expd)) {
    std::cout << "INPUT GRAMMAR:\n" << cfg::to_string(&input, &e) << std::endl;
    std::cout << "PROCESSED GRAMMAR:\n" << cfg::to_string(&gv, &e) << std::endl;
    std::cout << "EXPECTED GRAMMAR:\n"
              << cfg::to_string(&expd, &e) << std::endl;
    return 4;
  }
}
