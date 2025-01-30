#pragma once

#include <cfgtk/common.hpp>
#include <cfgtk/filter.hpp>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>

namespace cfg {

struct text_encoding {
  std::string rule_separator{" -> "};
  std::string rhs_separator{" "};
  bool lhs_padding{true};
};

struct cpp_encoding {
  std::string grammar_name{"g"};
  std::string action_map_name{};
  std::string action_name{};
  bool namespace_on{true};
};

struct rule {
  symbol_t lhs{};
  std::vector<symbol_t> rhs{};
};
using grammar_t = std::vector<std::unique_ptr<rule>>;

struct inclusive_range {
  std::size_t begin{};
  std::size_t end{};
};

struct rule_info {
  const rule *entry{};
  inclusive_range tokens{};
};

struct chart_node {
  struct callback {
    const cfg::rule *key{};
    chart_node *lhs{}, *left{}, *right{};
  };

  std::list<callback> actions{};
  std::string value{};
  rule_info rule{};
  chart_node *head{};
  chart_node *tail{};
};

struct rule_match_info {
  std::list<chart_node> nodes{};
  inclusive_range head_tokens{};
  inclusive_range tail_tokens{};
};

using chart_t = std::vector<std::vector<rule_match_info>>;
using action_t = std::function<void(chart_node *, chart_node *, chart_node *)>;
using action_map_t = std::unordered_map<const rule *, std::list<action_t>>;

void run_actions(const chart_node *, const action_map_t *);

inline rule *add_rule(grammar_t *g, symbol_t lhs) {
  g->push_back(std::unique_ptr<rule>{new rule{std::move(lhs)}});
  return g->back().get();
}

inline rule *add_rule(grammar_t *g, symbol_t lhs,
                              std::vector<symbol_t> rhs) {
  g->push_back(std::unique_ptr<rule>{new rule{std::move(lhs), std::move(rhs)}});
  return g->back().get();
}

template <typename... P>
  requires(std::convertible_to<P, symbol_t> && ...)
inline rule *add_rule(grammar_t *g, symbol_t lhs, P &&...rhs) {
  g->push_back(std::unique_ptr<rule>{
      new rule{std::move(lhs), {std::forward<P>(rhs)...}}});
  return g->back().get();
}

template <typename F>
  requires std::convertible_to<F, action_t>
std::list<action_t>::iterator bind(action_map_t *m, rule *r, F &&a) {
  if (!m->contains(r))
    m->emplace(r, std::list<action_t>{std::forward<F>(a)});
  else {
    auto& ref = m->at(r);
		ref.push_back(std::forward<F>(a));
	}
  return --(m->at(r).end());
}

inline void unbind(action_map_t *m, rule *r, std::list<action_t>::iterator it) {
  if (!m)
    return;
  m->at(r).erase(it);
}

using token_sequence_t = std::vector<token_t>;

chart_t cyk(const grammar_t *, const token_sequence_t *,
            const action_map_t * = nullptr);

enum class cnf_filter : unsigned {
  unique0 = 1 << 0,
  start = 1 << 1,
  term = 1 << 2,
  bin = 1 << 3,
  del = 1 << 4,
  unique1 = 1 << 5,
  unit = 1 << 6,
  unique2 = 1 << 7,
  group = 1 << 8,
  random = 1 << 9,
  prune = 1 << 10
};

inline constexpr cnf_filter operator|(cnf_filter a, cnf_filter b) {
  return static_cast<cnf_filter>(static_cast<unsigned>(a) |
                                 static_cast<unsigned>(b));
}

inline constexpr cnf_filter operator&(cnf_filter a, cnf_filter b) {
  return static_cast<cnf_filter>(static_cast<unsigned>(a) &
                                 static_cast<unsigned>(b));
}

struct cnf_info {
  cnf_filter filter{cnf_filter::unique0 | cnf_filter::start | cnf_filter::term |
                    cnf_filter::bin | cnf_filter::del | cnf_filter::unique1 |
                    cnf_filter::unit | cnf_filter::unique2 | cnf_filter::group |
                    cnf_filter::random | cnf_filter::prune};
};

result to_cnf(const grammar_t *input, grammar_t *out, const cnf_info *);

template <typename G> inline symbol_t get_start(const G *grammar) {
  return grammar->front()->lhs;
}

bool is_valid(const chart_t *c, const symbol_t &start);
bool is_valid(const chart_node *c, const symbol_t &start);
bool is_equal(const grammar_t *g1, const grammar_t *g2);

std::string to_string(const chart_t *);

std::string to_string(const chart_node *, const token_sequence_t *,
                      const std::size_t vertical_spacing = 1,
                      const std::size_t horizontal_shift = 0,
                      const char shift_prefix = '\t');

std::string to_string(const rule *, const std::size_t padding,
                      const std::string &separator = " -> ",
                      const bool apply_to_all = false);

std::string to_string(const grammar_t *, const text_encoding *);
std::string to_string(const grammar_t *, const cpp_encoding *);
std::string to_string(const std::vector<const rule *> *, const text_encoding *);

std::vector<chart_node> get_trees(const chart_t *, const symbol_t &start);

result read_from_file(const std::string &path, grammar_t *);
} // namespace cfg
