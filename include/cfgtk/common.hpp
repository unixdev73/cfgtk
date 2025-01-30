#pragma once

#include <fstream>
#include <string>

namespace cfg {

template <typename Symbol> struct token {
  Symbol id{};
  std::string value{};
};

enum class result {
  success,
  match_failure,
  file_access_failure,
  excessive_symbols,
  format_error,
  bad_arg_type
};

using symbol_t = std::string;
using token_t = token<symbol_t>;

inline result write_to_file(const std::string &p, const std::string &d) {
  if (d.size()) {
    std::ofstream str{p};
    if (!str.is_open())
      return cfg::result::file_access_failure;
    str << d;
  }
  return cfg::result::success;
}
} // namespace cfg
