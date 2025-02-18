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

  cfg::grammar_t expd{}, input{}, out{};
  std::vector<cfg::grammar_t *> gseq{&expd, &input};
  for (std::size_t i = 0; i < gseq.size(); ++i) {
    const std::size_t index = i + 1;
    if (cfg::read_from_file(argv[index], gseq[i]) != cfg::result::success) {
      std::cerr << "Reading grammar[" << index << "]  at: '" << argv[index]
                << "' failed.\n";
      return 2;
    }
  }

  cfg::cnf_info info{};
  info.filter = cfg::cnf_filter::unique0 | cfg::cnf_filter::reduce |
                cfg::cnf_filter::unique2;
  cfg::to_cnf(&input, &out, &info);

  cfg::text_encoding e{};
  if (!is_equal(&out, &expd)) {
    std::cout << "INPUT GRAMMAR:\n" << cfg::to_string(&input, &e) << std::endl;
    std::cout << "PROCESSED GRAMMAR:\n"
              << cfg::to_string(&out, &e) << std::endl;
    std::cout << "EXPECTED GRAMMAR:\n"
              << cfg::to_string(&expd, &e) << std::endl;
    return 3;
  }
}
