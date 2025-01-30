#pragma once

#include <cfgtk/common.hpp>
#include <cfgtk/filter.hpp>
#include <ostream>
#include <regex>
#include <string>
#include <vector>

namespace cfg {
enum class token_type { option, flag, free };

struct lexer_entry {
  lexer_entry(token_type t, std::string i, std::string p)
      : type{t}, id{std::move(i)}, pattern{p}, regex{std::move(p)} {}

  token_type type{};
  symbol_t id{};
  std::regex pattern{};
  std::string regex{};
};

using lexer_table_t = std::vector<lexer_entry>;
using lexer_input_t = std::vector<std::string>;

std::vector<token_t> tokenize(const lexer_table_t *, const lexer_input_t *);

inline void add_entry(lexer_table_t *tbl, const token_type &t,
                      const std::string &id, const std::string &rgx) {
  tbl->push_back({t, id, rgx});
}

result read_from_file(const std::string &src, lexer_table_t *dest);

std::string to_string(const lexer_table_t *);
} // namespace cfg

std::ostream &operator<<(std::ostream &o, const cfg::token_type &t);
