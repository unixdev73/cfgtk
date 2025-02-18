#include <cfgtk/parser.hpp>
#include <cmath>
#include <fstream>
#include <random>
#include <regex>
#include <set>
#include <sstream>

namespace {
using grammar_list_t =
    decltype(flt::to_container<std::list>(std::declval<cfg::grammar_t>()));
}

namespace {
inline std::size_t make_random(std::size_t begin, std::size_t end) {
  static thread_local std::mt19937 rng{std::random_device{}()};
  return begin + rng() % end;
}

template <template <typename> typename C>
C<std::pair<cfg::chart_node *, cfg::chart_node *>>
cartesian_product(C<cfg::chart_node> &A, C<cfg::chart_node> &B) {
  C<std::pair<cfg::chart_node *, cfg::chart_node *>> p{};
  if constexpr (std::same_as<C<std::pair<cfg::chart_node *, cfg::chart_node *>>,
                             std::vector<std::pair<cfg::chart_node *,
                                                   cfg::chart_node *>>>)
    p.reserve(A.size() * B.size());

  for (auto &a : A)
    for (auto &b : B)
      p.push_back(std::pair(&a, &b));
  return p;
}

void recognize(const cfg::grammar_t *g, const cfg::token_t &s,
               const cfg::action_map_t *m, const std::size_t i,
               std::list<cfg::chart_node> &nodes) {

  for (const auto &r : *g) {
    if (r->rhs.size() == 1 && r->rhs.front() == s.id) {
      nodes.push_back({});
      auto &b = nodes.back();
      b.value = s.value;
      b.rule = {.entry = r.get(), .tokens = {i, i}};

      if (m && m->contains(r.get()))
        b.actions.push_back({r.get(), &b, nullptr, nullptr});
    }
  }
}

void recognize(const cfg::grammar_t *g,
               std::pair<cfg::chart_node *, cfg::chart_node *> p,
               const cfg::action_map_t *m, std::list<cfg::chart_node> &nodes) {

  const auto &s2 = p.second->rule.entry->lhs;
  const auto &s1 = p.first->rule.entry->lhs;

  for (const auto &r : *g) {
    if (r->rhs.size() == 2 && r->rhs.front() == s1 && r->rhs.back() == s2) {
      cfg::chart_node n{};
      n.rule.tokens.begin = p.first->rule.tokens.begin;
      n.rule.tokens.end = p.second->rule.tokens.end;
      n.head = p.first;
      n.tail = p.second;
      n.rule.entry = r.get();

      n.actions = p.first->actions;
      for (const auto &a : p.second->actions)
        n.actions.push_back(a);
      nodes.push_back(std::move(n));

      auto &b = nodes.back();
      if (m && m->contains(r.get()))
        b.actions.push_back({r.get(), &b, p.first, p.second});
    }
  }
}

const cfg::rule *is_empty_ok(const cfg::grammar_t *g,
                                     const cfg::symbol_t &start) {
  for (const auto &r : *g)
    if (r->lhs == start && !r->rhs.size())
      return r.get();
  return nullptr;
}

bool handle_early_exit(const cfg::grammar_t *g, const std::size_t tok_size,
                       cfg::chart_t &c, const cfg::action_map_t *m) {
  if (tok_size)
    return false;

  if (auto r = is_empty_ok(g, get_start(g)); r) {
    c.push_back({{{.nodes = {{.rule = r}}}}});
    if (m && m->contains(r)) {
      auto &node = c.back().front().nodes.front();
      node.actions.push_back({.key = r, .lhs = &node});
    }
  }

  return true;
}

bool initialize(const cfg::grammar_t *g, const cfg::token_sequence_t *t,
                const cfg::action_map_t *m, cfg::chart_t &c) {
  if (handle_early_exit(g, t->size(), c, m))
    return false;

  c.resize(t->size());
  for (auto &row : c)
    row.resize(t->size());

  for (std::size_t i = 0; i < t->size(); ++i) {
    recognize(g, (*t)[i], m, i, c.front()[i].nodes);
    c[0][i].head_tokens = {i, i};
  }

  return true;
}

void parse_cyk_iteration(const cfg::grammar_t *g, const std::size_t row,
                         const std::size_t col, const std::size_t i,
                         cfg::chart_t &c, const cfg::action_map_t *m) {

  auto &diag = c[row - i - 1][col + i + 1].nodes;
  auto &vert = c[i][col].nodes;

  for (auto p : cartesian_product(vert, diag)) {
    const auto old = c[row][col].nodes.size();
    recognize(g, p, m, c[row][col].nodes);
    if (c[row][col].nodes.size() > old) {
      c[row][col].tail_tokens = {col + i + 1, col + row};
      c[row][col].head_tokens = {col, col + i};
    }
  }
}
} // namespace

namespace cfg {
chart_t cyk(const grammar_t *g, const token_sequence_t *t,
            const action_map_t *m) {
  chart_t c{};

  if (g && initialize(g, t, m, c))
    for (std::size_t row = 0; row < c.size(); ++row)
      for (std::size_t col = 0; col < c.size() - row; ++col)
        for (std::size_t i = 0; i < row; ++i)
          parse_cyk_iteration(g, row, col, i, c, m);

  return c;
}

bool is_valid(const chart_t *c, const symbol_t &start) {
  if (c && c->size()) {
    // Only the first cell of the root of the chart should be non-empty
    for (std::size_t i = 1; i < c->back().size(); ++i)
      if (c->back()[i].nodes.size())
        return false;

    // And it must contain the start symbol
    const auto &root = c->back().front().nodes;
    for (const auto &n : root)
      if (n.rule.entry->lhs == start)
        return true;
  }
  return false;
}

bool is_valid(const chart_node *n, const symbol_t &start) {
  return n ? n->rule.entry->lhs == start : false;
}

bool is_equal(const rule *r1, const rule *r2) {
  bool match{r1->rhs.size() == r2->rhs.size()};
  if (match)
    for (std::size_t i = 0; i < r1->rhs.size(); ++i)
      if (r1->rhs[i] != r2->rhs[i]) {
        match = false;
        break;
      }
  return r1->lhs == r2->lhs && match;
}

bool is_equal(const chart_node *a, const chart_node *b) {
  return a->rule.entry == b->rule.entry &&
         a->rule.tokens.begin == b->rule.tokens.begin &&
         a->rule.tokens.end == b->rule.tokens.end;
}

bool is_equal(const grammar_t *g1, const grammar_t *g2) {
  if (g1->size() != g2->size())
    return false;
  for (std::size_t i = 0; i < g1->size(); ++i)
    if (!is_equal((*g1)[i].get(), (*g2)[i].get()))
      return false;
  return true;
}

} // namespace cfg

namespace {
auto max_col_widths(const cfg::chart_t *c, const std::size_t v = 0) {
  std::vector<std::size_t> widths{};
  widths.resize(c->size(), 0);

  for (const auto &row : *c)
    for (std::size_t col = 0; col < row.size(); ++col)
      for (const auto &node : row[col].nodes)
        if (auto size = node.rule.entry->lhs.size(); size > widths[col])
          widths[col] = size;

  if (v)
    for (auto &w : widths)
      w += v;
  return widths;
}

std::size_t max_lhs_in_cell(const cfg::chart_t *c, const std::size_t row) {
  std::size_t m{};
  for (const auto &col : (*c)[row])
    if (col.nodes.size() > m)
      m = col.nodes.size();
  return m;
}

std::string make_chart_line(const std::size_t col_size,
                            const std::vector<std::size_t> &max_width) {

  std::string l{};
  for (std::size_t col = 0; col < col_size; ++col) {
    std::string f{};
    while (f.size() < max_width[col])
      f += " ";

    if (col < col_size - 1)
      f += "|";

    l += std::move(f);
  }

  return l;
}

std::string make_chart_line(const cfg::chart_t *c, const std::size_t row,
                            const std::size_t line,
                            const std::vector<std::size_t> &max_width) {

  std::string l{};
  for (std::size_t col = 0; col < (*c)[row].size(); ++col) {
    std::string f{}, b{}, e{}, s{};
    if ((*c)[row][col].nodes.size() > line) {
      auto node = (*c)[row][col].nodes.begin();
      for (std::size_t i = 0; i < line; ++i)
        ++node;
      b = std::to_string(node->rule.tokens.begin);
      e = std::to_string(node->rule.tokens.end);
      s = node->rule.entry->lhs;
    }

    const std::size_t not_padding = 1 + b.size() + 1 + e.size() + 2 + s.size();
    const std::size_t padding = (max_width[col] - not_padding) / 2;

    for (std::size_t i = 0; i < padding; ++i)
      f += " ";
    if (s.size())
      f += "(" + b + "," + e + ") " + s;
    while (f.size() < max_width[col])
      f += " ";

    if (col < (*c)[row].size() - 1)
      f += "|";
    l += std::move(f);
  }

  return l;
}

std::string make_separator(const std::size_t line_width) {
  std::string s{};
  s += '\n';
  for (std::size_t i = 0; i < line_width; ++i)
    s += '-';
  s += '\n';
  return s;
}
} // namespace

namespace cfg {
std::string to_string(const cfg::chart_t *c) {
  if (!c->size())
    return {};

  const auto e = std::to_string(c->front().back().head_tokens.end);
  const auto col_widths = max_col_widths(c, 2 * e.size() + 6);
  std::stringstream s{};

  for (std::size_t row = 0; row < c->size(); ++row) {
    std::size_t line_width{};
    std::string line{};

    if (const auto max_lhs = max_lhs_in_cell(c, row); max_lhs) {
      for (std::size_t li = 0; li < max_lhs; ++li) {
        s << (line = make_chart_line(c, row, li, col_widths));
        if (li < max_lhs - 1)
          s << '\n';
      }
    } else
      s << (line = make_chart_line((*c)[row].size(), col_widths));

    line_width = line.size();
    if (row == c->size() - 1)
      continue;
    s << make_separator(line_width);
  }

  return s.str();
}
} // namespace cfg

namespace {

template <typename G>
std::size_t count_sym(const G &grammar, const cfg::symbol_t &s) {
  std::size_t occ{};
  for (const auto &r : grammar) {
    if (r->lhs == s)
      ++occ;
    for (const auto &sym : r->rhs)
      if (sym == s)
        ++occ;
  }

  return occ;
}

template <typename G>
std::size_t count_sym(const G &grammar, const std::regex &s) {
  std::size_t occ{};
  for (const auto &r : grammar) {
    if (std::regex_match(r->lhs, s))
      ++occ;
    for (const auto &sym : r->rhs)
      if (std::regex_match(sym, s))
        ++occ;
  }

  return occ;
}

template <template <typename> typename C, typename T>
  requires requires(T t) {
    requires std::convertible_to<std::string, decltype(t->lhs)>;
  }
std::string make_unique_id(const C<T> &c, std::string prefix, bool random) {
  using std::stoull;
  auto hashpos = std::find(prefix.begin(), prefix.end(), '#');
  const bool hash = hashpos != prefix.end();
  const auto oc = count_sym(c, std::regex{"^" + prefix + ".*$"});
  const std::size_t idlen = oc ? std::log10(oc) + 1 : 1;
  static const std::string sset{"0123456789"};
  std::size_t index{hash ? stoull(std::string{++hashpos, prefix.end()}) : 0};
  std::string id{};

  if (hash)
    while (hashpos != prefix.end())
      prefix.pop_back();

  do {
    std::string postfix{hash ? "" : "#"};
    if (random)
      for (std::size_t i = 0; i < idlen; ++i)
        postfix += sset[make_random(0, sset.size())];
    else
      postfix += std::to_string(index++);
    id = prefix + postfix;
  } while (count_sym(c, id));

  return id;
}
} // namespace

namespace {
template <typename C> std::set<std::string> get_nonterms(const C &grammar) {
  std::set<std::string> out{};
  for (const auto &r : grammar)
    out.emplace(r->lhs);
  return out;
}
} // namespace

namespace {
void rm_explicit_nullable(grammar_list_t &glist) {
  const auto start = cfg::get_start(&glist);
  auto it = glist.begin();
  while (it != glist.end()) {
    if (!(*it)->rhs.size() && (*it)->lhs != start)
      it = glist.erase(it);
    else
      ++it;
  }
}

void rm_unreachable_nterms(grammar_list_t &glist,
                           const std::set<std::string> &nterm,
                           const std::set<std::string> &nterm2) {
  for (auto it = glist.begin(); it != glist.end();) {
    if (!(*it)->rhs.size()) {
      ++it;
      continue;
    }

    bool unreachable = nterm.contains((*it)->rhs.front()) &&
                       !nterm2.contains((*it)->rhs.front());

    if (!unreachable && (*it)->rhs.size() == 2)
      unreachable = nterm.contains((*it)->rhs.back()) &&
                    !nterm2.contains((*it)->rhs.back());

    if (unreachable)
      it = glist.erase(it);
    else
      ++it;
  }
}

std::set<cfg::symbol_t> get_nullable(grammar_list_t &glist) {
  const auto start = cfg::get_start(&glist);
  std::set<cfg::symbol_t> nullable{};

  for (const auto &rule : glist)
    if (rule->lhs != start && !rule->rhs.size())
      nullable.emplace(rule->lhs);

  for (const auto &rule : glist) {
    bool append{true};
    for (const auto &sym : rule->rhs)
      if (!nullable.contains(sym)) {
        append = false;
        break;
      }

    if (append)
      nullable.emplace(rule->lhs);
  }

  return nullable;
}

std::vector<std::vector<cfg::symbol_t>>
get_rhs(const grammar_list_t &glist, const std::string lhs,
        const std::vector<cfg::symbol_t> &exclude) {
  std::vector<std::vector<cfg::symbol_t>> out{};

  for (const auto &rule : glist) {
    bool ok{true};
    for (const auto &e : exclude) {
      if (rule->rhs.size() == 1 && rule->rhs.front() == e) {
        ok = false;
        break;
      }
    }

    if (rule->lhs == lhs && ok)
      out.push_back(rule->rhs);
  }

  return out;
}
} // namespace

namespace cfg {
// Eliminate duplicate rule definitions while preserving
// the first definition of the start symbol.
void make_unique(grammar_list_t &glist) {
  auto it = ++glist.begin();
  while (it != glist.end()) {
    bool unique{true};
    for (auto it2 = glist.begin(); it2 != glist.end(); ++it2) {
      if (it == it2)
        continue;
      if (is_equal(it->get(), it2->get())) {
        unique = false;
        break;
      }
    }

    if (!unique)
      it = glist.erase(it);
    else
      ++it;
  }
}

// Introduce a new, unique start symbol
// if the current start symbol appears
// on the right-hand side of any rule.
void to_cnf_start(grammar_list_t &glist, bool random) {
  auto start = get_start(&glist);

  auto scan_rhs = [&glist](const symbol_t &s) -> bool {
    return std::find_if(glist.begin(), glist.end(), [&s](const auto &e) {
             return std::find(e->rhs.begin(), e->rhs.end(), s) != e->rhs.end();
           }) != glist.end();
  };

  if (scan_rhs(start))
    glist.push_front(std::unique_ptr<rule>{
        new rule{make_unique_id(glist, start, random), {start}}});
}

// If a terminal symbol appears alongside
// non-terminal symbols on the right-hand side
// of a rule, create a new non-terminal rule
// to represent the terminal and substitute it.
void to_cnf_term(grammar_list_t &glist, bool random) {
  auto nterm = get_nonterms(glist);
  auto isterm = [&nterm](const std::string &s) { return !nterm.contains(s); };

  auto replace = [isterm, &glist, random](std::vector<symbol_t> &rhs) {
    if (rhs.size() < 2)
      return;

    for (auto &s : rhs)
      if (isterm(s)) {
        auto new_s = make_unique_id(glist, s, random);
        glist.push_back(std::unique_ptr<rule>{new rule{new_s, {s}}});
        s = std::move(new_s);
      }
  };

  for (auto &r : glist)
    replace(r->rhs);
}

// Introduce new binary rules for any production with
// more than two non-terminal symbols on the right-hand
// side; recursively transform until all rules have at
// most two symbols on the right-hand side.
void to_cnf_bin(grammar_list_t &glist, bool random) {
  for (auto &r : glist) {
    rule copy_rule{*r};
    auto &current = copy_rule;
    bool first{true};

    while (current.rhs.size() >= 3) {
      rule new_rule{.lhs = make_unique_id(glist, current.lhs, random)};

      const std::size_t new_size{current.rhs.size() - 1};
      new_rule.rhs.resize(new_size);
      for (std::size_t i = new_size; i > 0; --i) {
        new_rule.rhs[i - 1] = std::move(current.rhs[i]);
        current.rhs.pop_back();
      }
      current.rhs.push_back(new_rule.lhs);

      if (first) {
        *r = std::move(current);
        first = false;
      } else
        glist.push_back(std::unique_ptr<rule>{new rule{std::move(current)}});

      copy_rule = std::move(new_rule);
      current = copy_rule;
    }

    if (!first)
      glist.push_back(std::unique_ptr<rule>{new rule{std::move(current)}});
  }
}

// Remove rules with empty right-hand sides and replace them with
// variants where the empty symbol is appended as needed.
result to_cnf_del(grammar_list_t &glist, bool prune) {
  const auto start = get_start(&glist);
  const auto nullable = get_nullable(glist);
  if (!nullable.size() || (nullable.size() == 1 && *nullable.begin() == start))
    return result::success;

  auto nterm = get_nonterms(glist);

  for (auto r = glist.begin(); r != glist.end(); ++r) {
    if ((*r)->rhs.size() > 2)
      return result::excessive_symbols;
    if (!(*r)->rhs.size())
      continue;
    if ((*r)->rhs.size() == 1) {
      if (nullable.contains((*r)->rhs.front()))
        glist.push_back(std::unique_ptr<rule>{new rule{(*r)->lhs, {}}});
    } else {
      bool x1{false}, x2{false};
      if (nullable.contains((*r)->rhs.front())) {
        glist.push_back(
            std::unique_ptr<rule>{new rule{(*r)->lhs, {(*r)->rhs.back()}}});
        x1 = true;
      }
      if (nullable.contains((*r)->rhs.back())) {
        glist.push_back(
            std::unique_ptr<rule>{new rule{(*r)->lhs, {(*r)->rhs.front()}}});
        x2 = true;
      }
      if (x1 && x2)
        glist.push_back(std::unique_ptr<rule>{new rule{(*r)->lhs, {}}});
    }
  }

  rm_explicit_nullable(glist);
  if (prune) {
    auto nterm2 = get_nonterms(glist);
    rm_unreachable_nterms(glist, nterm, nterm2);
  }
  return result::success;
}

// Eliminate unit rules where a non-terminal symbol
// directly maps to another non-terminal symbol.
void to_cnf_unit(grammar_list_t &glist) {
  const auto nterms = get_nonterms(glist);
  for (auto r = glist.begin(); r != glist.end();) {
    if ((*r)->rhs.size() != 1 || !nterms.contains((*r)->rhs.front())) {
      ++r;
      continue;
    }

    const std::vector<symbol_t> exclude{(*r)->rhs.front(), (*r)->lhs};
    bool inserted{false};
    for (auto &&rhs : get_rhs(glist, (*r)->rhs.front(), exclude)) {
      if (!inserted) {
        (*r)->rhs = std::move(rhs);
        inserted = true;
      } else
        glist.push_back(
            std::unique_ptr<rule>{new rule{(*r)->lhs, std::move(rhs)}});
    }

    if (inserted)
      r = glist.begin();
    else
      ++r;
  }
}

// Eliminate rules that only have one right hand side that is not unique;
// then replace the occurrence of their left hand sides with the first rule
// with the same right hand side
void to_cnf_reduce(grammar_list_t &glist) {
  bool removed{false};
  do {
    removed = false;

    std::unordered_map<std::string, std::size_t> occmap{};
    for (auto it = glist.begin(); it != glist.end(); ++it) {
      if (!occmap.contains((*it)->lhs))
        occmap.emplace((*it)->lhs, 1);
      else
        ++occmap.at((*it)->lhs);
    }

    std::list<std::list<std::unique_ptr<rule>>::iterator> ids{};
    std::unordered_map<std::string, std::string> replace{};
    for (auto it = glist.begin(); it != glist.end();) {
      if ((*it)->rhs.size() && occmap.at((*it)->lhs) == 1) {
        auto it2 = ids.begin();
        for (; it2 != ids.end(); ++it2)
          if ((*(*it2))->rhs == (*it)->rhs &&
              occmap.at((*it2)->get()->lhs) == 1)
            break;
        if (it2 == ids.end())
          ids.push_back(it);
        else if (it != *it2) {
          replace.emplace((*it)->lhs, (*(*it2))->lhs);
          it = glist.erase(it);
          removed = true;
          continue;
        }
      }
      ++it;
    }

    if (replace.size()) {
      for (auto it = glist.begin(); it != glist.end(); ++it)
        for (auto &s : (*it)->rhs)
          if (replace.contains(s))
            s = replace.at(s);
    }
  } while (removed);
}
} // namespace cfg

namespace {
// Purely optional, reorder the final grammar
// in such a way that any rules with the same lhs
// are next to each other.
void group_rules(grammar_list_t &g) {
  std::unordered_map<cfg::symbol_t, decltype(g.begin())> last_occ{};
  std::list<decltype(g.begin())> for_removal{};

  for (auto i = g.begin(); i != g.end();) {
    if (!last_occ.contains((*i)->lhs)) {
      last_occ.emplace((*i)->lhs, i);
      ++i;
      continue;
    }

    auto before = ++last_occ.at((*i)->lhs);
    auto &e = last_occ.at((*i)->lhs);
    e = g.insert(before, std::move(*i));
    i = g.erase(i);
  }
}
} // namespace

namespace cfg {
result to_cnf(const grammar_t *input, grammar_t *out, const cnf_info *info) {
  if (!input || !input->size() || !out || !info)
    return {};

  const bool rm_unreachable = bool(info->filter & cnf_filter::prune);
  const bool random_id = bool(info->filter & cnf_filter::random);
  auto g = flt::to_container<std::list>(*input);

  if (bool(info->filter & cnf_filter::unique0))
    make_unique(g);
  if (bool(info->filter & cnf_filter::start))
    to_cnf_start(g, random_id);
  if (bool(info->filter & cnf_filter::term))
    to_cnf_term(g, random_id);
  if (bool(info->filter & cnf_filter::bin))
    to_cnf_bin(g, random_id);
  if (bool(info->filter & cnf_filter::del))
    if (auto r = to_cnf_del(g, rm_unreachable); r != result::success)
      return r;
  if (bool(info->filter & cnf_filter::unique1))
    make_unique(g);
  if (bool(info->filter & cnf_filter::unit))
    to_cnf_unit(g);
  if (bool(info->filter & cnf_filter::unique2))
    make_unique(g);
  if (bool(info->filter & cnf_filter::reduce))
    to_cnf_reduce(g);
  if (bool(info->filter & cnf_filter::group))
    group_rules(g);

  for (auto &&r : g)
    out->push_back(std::move(r));
  return result::success;
}

std::vector<chart_node> get_trees(const chart_t *c, const symbol_t &start) {
  if (!c || !c->size())
    return {};
  std::vector<chart_node> trees{};
  const auto &root = c->back().front();
  trees.reserve(root.nodes.size());

  for (const auto &n : root.nodes)
    if (n.rule.entry->lhs == start)
      trees.push_back(n);
  return trees;
}
}

namespace cfg {
std::string to_string(const rule *r, const std::size_t padding,
                      const std::string &rule_sep, const std::string &rhs_sep) {
  std::string out{};
  // Handle LHS
  out += r->lhs;
  if (padding > r->lhs.size())
    for (std::size_t i = 0; i < padding - r->lhs.size(); ++i)
      out += " ";
  out += rule_sep;

  // Handle RHS
  for (const auto &sym : r->rhs)
    out += sym + rhs_sep;
  for (std::size_t i = 0; i < rhs_sep.size(); ++i)
    out.pop_back();
  return out;
}

std::size_t max_lhs_size(const grammar_t *g) {
  std::size_t max{g->front()->lhs.size()};
  for (const auto &r : *g)
    if (r->lhs.size() > max)
      max = r->lhs.size();
  return max;
}

std::string to_string(const grammar_t *g, const text_encoding *e) {
  if (!g || !g->size() || !e)
    return {};

  const std::size_t p{e->lhs_padding ? max_lhs_size(g) : 0};
  std::string out{};

  for (const auto &r : *g)
    out += to_string(r.get(), p, e->rule_separator, e->rhs_separator) + "\n";

  out.pop_back();
  return out;
}

std::string to_string(const grammar_t *gr, const cpp_encoding *e) {
  if (!gr || !gr->size() || !e)
    return {};

  const std::string g{e->grammar_name.size() ? e->grammar_name : "g"},
      m{e->action_map_name}, n{e->namespace_on ? "cfg::" : ""}, a{"bind(&"},
      r{"add_rule(&"}, c{"action_map_t"}, s{"action_t"}, t{"grammar_t"};

  const bool action_variant{m.size() > 0};
  std::string out{}, f{e->action_name.size() ? e->action_name : n + s + "{}"};

  if (action_variant)
    out += n + c + " " + m + "{};\n";

  out += n + t + " " + g + "{};\n";

  if (action_variant && e->action_name.size())
    out += n + s + " " + f + "{};\n";

  for (const auto &entry : *gr) {
    if (action_variant)
      out += n + a + m + ", ";

    out += n + r + g + ", \"" + entry->lhs + "\", ";
    for (const auto &rhs : entry->rhs)
      out += "\"" + rhs + "\", ";
    out.pop_back();
    out.pop_back();

    if (action_variant)
      out += "), " + f + ");\n";
    else
      out += ");\n";
  }

  out.pop_back();
  return out;
}
} // namespace cfg

namespace {
struct node_info {
  const cfg::chart_node *node{};
  std::size_t row, col;
};

struct tree_info {
  std::size_t height{}, width{};
  std::vector<node_info> nodes{};
};

tree_info serialize(const cfg::chart_node *root) {
  std::list<node_info> queue{{root, 0, 0}};
  std::size_t max_height{}, max_width{};
  std::vector<node_info> previous{};

  while (queue.size()) {
    auto n = queue.front();
    queue.pop_front();

    for (const auto &p : previous) {
      if (p.node->head == n.node || p.node->tail == n.node) {
        n.col = p.col + 1;
        break;
      }
    }
    n.row = max_height;

    if (n.node->tail)
      queue.push_front({n.node->tail, 0, 0});
    if (n.node->head)
      queue.push_front({n.node->head, 0, 0});
    else
      ++max_height;

    if (n.col > max_width)
      max_width = n.col;

    previous.push_back(n);
  }

  return {max_height, max_width + 1, std::move(previous)};
}

std::vector<std::vector<const cfg::chart_node *>>
make_node_table(const tree_info &tree) {
  std::vector<std::vector<const cfg::chart_node *>> table{};

  table.resize(tree.height);
  for (auto &row : table)
    row.resize(tree.width);

  for (std::size_t i = 0; i < tree.nodes.size(); ++i)
    table[tree.nodes[i].row][tree.nodes[i].col] = tree.nodes[i].node;

  return table;
}

std::vector<std::size_t>
make_size_table(const std::vector<std::vector<const cfg::chart_node *>> &n) {

  if (!n.size() || !n.front().size())
    return {};

  std::vector<std::vector<std::size_t>> table{};
  table.resize(n.size());
  for (auto &row : table)
    row.resize(n.front().size());

  std::vector<std::size_t> widths{};
  widths.reserve(table.front().size());

  for (std::size_t col = 0; col < table.front().size(); ++col) {
    std::size_t max{};
    for (std::size_t row = 0; row < table.size(); ++row)
      if (n[row][col] && n[row][col]->rule.entry->lhs.size() > max)
        max = n[row][col]->rule.entry->lhs.size();
    widths[col] = max;
  }

  return widths;
}

std::string make_fields(const std::vector<const cfg::chart_node *> &row,
                        const std::vector<std::size_t> &sizes, std::size_t &col,
                        const std::string &separator) {

  std::string out{};

  for (; col < row.size() && row[col]; ++col) {
    const bool next_is_null = col < row.size() - 1 && !row[col + 1];
    const auto label = row[col]->rule.entry->lhs;
    std::string field(sizes[col], ' ');
    for (std::size_t i = 0; i < label.size(); ++i)
      field[i] = label[i];

    if (next_is_null)
      out += field + " " + std::string(separator.size() - 1, '-');
    else
      out += field + separator;
  }

  return out;
}

std::string make_prefix(const std::vector<const cfg::chart_node *> &row,
                        const std::vector<std::size_t> &sizes,
                        std::size_t &index, const std::string &separator) {

  std::string out{};

  for (; index < row.size() && !row[index]; ++index)
    out += std::string(sizes[index] + separator.size(), ' ');

  return out;
}

std::string make_postfix(const std::vector<const cfg::chart_node *> &row,
                         const std::vector<std::size_t> &sizes,
                         std::size_t &col, const std::string &separator) {

  std::string record{};
  bool right_empty{false};

  for (; col < row.size() && !row[col]; ++col) {
    record += std::string(sizes[col] + separator.size(), '-');
    right_empty = true;
  }

  if (right_empty) {
    record.pop_back();
    record.pop_back();
    record.append("> ");
  }

  return record;
}
} // namespace

namespace {
std::string make_record(const std::vector<const cfg::chart_node *> &row,
                        const std::vector<std::size_t> &sizes,
                        const std::vector<cfg::token_t> *tokens) {

  static const std::string separator{" -> "};
  std::size_t col{};

  std::string record = make_prefix(row, sizes, col, separator);
  record += make_fields(row, sizes, col, separator);
  record += make_postfix(row, sizes, col, separator);

  for (; col > 0 && !row[--col];)
    continue;

  if (row[col] && tokens && tokens->size()) {
    const auto i = row[col]->rule.tokens.begin;
    record += (*tokens)[i].id + ": " + (*tokens)[i].value;
  }

  return record;
}
} // namespace

namespace cfg {
std::string to_string(const chart_node *root, const token_sequence_t *tokens,
                      const std::size_t vertical_spacing,
                      const std::size_t horizontal_shift,
                      const char shift_prefix) {
  if (!root || !root->rule.entry)
    return {};

  const auto node_table = make_node_table(serialize(root));
  const auto size_table = make_size_table(node_table);
  std::string out{};

  for (std::size_t row = 0; row < node_table.size(); ++row) {
    auto hshift = std::string(horizontal_shift, shift_prefix);
    auto record = hshift + make_record(node_table[row], size_table, tokens);
    auto blanks = static_cast<decltype(record.size())>(
        std::count(record.begin(), record.end(), ' '));
    if (bool transparent = blanks == record.size(); transparent)
      continue;

    out += std::move(record);

    if (row < node_table.size() - 1)
      for (std::size_t i = 0; i < vertical_spacing; ++i)
        out += "\n";
  }

  return out;
}
} // namespace cfg

namespace cfg {
result read_from_file(const std::string &path, grammar_t *g) {
  std::ifstream str{path};
  if (!str.is_open())
    return result::file_access_failure;

  std::string record{};
  while (std::getline(str, record)) {
    if (!record.size())
      continue;

    auto fields = flt::split<std::vector>(record, " ");
    std::vector<symbol_t> rhs{};
    rhs.reserve(fields.size());
    for (std::size_t i = 1; i < fields.size(); ++i)
      if (fields[i].size())
        rhs.push_back(std::move(fields[i]));

    if (g) {
      if (rhs.size())
        add_rule(g, std::move(fields[0]), std::move(rhs));
      else
        add_rule(g, std::move(fields[0]));
    }
  }

  return result::success;
}
} // namespace cfg

namespace cfg {
std::string to_string(const std::vector<const rule *> *s,
                      const text_encoding *e) {
  if (!s || !e)
    return {};

  std::vector<std::unique_ptr<rule>> rules{};
  rules.reserve(s->size());

  for (const auto &r : *s)
    rules.push_back(std::unique_ptr<rule>{new rule{*r}});

  return to_string(&rules, e);
}

std::vector<const rule *> get_free_rules(const chart_node *root,
                                         const action_map_t *m,
                                         bool skip_single) {
  if (!root || !m)
    return {};

  std::list<const chart_node *> queue{root};
  std::vector<const rule *> rules{};

  while (!queue.empty()) {
    auto n = queue.front();
    queue.pop_front();

    if (!m->contains(n->rule.entry))
      if (!skip_single || n->rule.entry->rhs.size() >= 2)
        rules.push_back(n->rule.entry);

    if (n->head)
      queue.push_back(n->head);
    if (n->tail)
      queue.push_back(n->tail);
  }

  return rules;
}

std::vector<const rule *>
get_free_rules(const grammar_t *g, const action_map_t *m, bool skip_single) {

  if (!g || !m)
    return {};

  std::vector<const rule *> rules{};
  for (const auto &n : *g)
    if (!m->contains(n.get()))
      if (!skip_single || n->rhs.size() >= 2)
        rules.push_back(n.get());

  return rules;
}

void run_actions(const chart_node *root, const action_map_t *m) {
  if (!root || !m)
    return;

  for (const auto &a : root->actions)
    if (m->contains(a.key))
      for (const auto &f : m->at(a.key))
        if (f)
          f(a.lhs, a.left, a.right);
}
} // namespace cfg
