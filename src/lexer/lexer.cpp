#include <cfgtk/lexer.hpp>
#include <fstream>
#include <list>
#include <sstream>

namespace cfg {
enum class lexer_mode { single_id, id_list, non_id, forced_non_id };

struct lexer_context {
  std::vector<token_t> token_sequence;
  lexer_mode mode;
};
} // namespace cfg

namespace {
cfg::lexer_mode detect_mode(const std::string &token);
cfg::result tokenize_single_id(const cfg::lexer_table_t *, const std::string &,
                               cfg::token_t *);
cfg::result tokenize_id_list(const cfg::lexer_table_t *, const std::string &,
                             std::vector<cfg::token_t> *&);
cfg::result tokenize_flag(const cfg::lexer_table_t *, const std::string &,
                          cfg::token_t *);
cfg::result tokenize_flag(const cfg::lexer_table_t *, const char,
                          cfg::token_t *);
} // namespace

namespace {
cfg::result handle_single_id(const cfg::lexer_table_t *tbl,
                             const std::string &inp, cfg::lexer_context &ctx) {
  cfg::token_t tok{};
  if (ctx.mode == cfg::lexer_mode::single_id) {
    if (tokenize_single_id(tbl, inp, &tok) == cfg::result::success) {
      ctx.token_sequence.push_back(std::move(tok));
      return cfg::result::success;
    }
  }
  return cfg::result::match_failure;
}

cfg::result handle_id_list(const cfg::lexer_table_t *tbl,
                           const std::string &inp, cfg::lexer_context &ctx) {
  if (ctx.mode == cfg::lexer_mode::id_list) {
    std::vector<cfg::token_t> *tseq = &ctx.token_sequence;
    if (tokenize_id_list(tbl, inp, tseq) == cfg::result::success)
      return cfg::result::success;
  }

  return cfg::result::match_failure;
}

void handle_non_id(const cfg::lexer_table_t *tbl, const std::string &val,
                   cfg::lexer_context &ctx) {
  std::string id{};

  // Since at this point the val string is not an option and not a flag,
  // we check if it matches a custom regex,
  // if not, then we simply insert an empty token_id with the value
  for (const auto &e : *tbl) {
    if (e.type != cfg::token_type::option && e.type != cfg::token_type::flag &&
        std::regex_match(val, e.pattern)) {
      id = e.id;
      break;
    }
  }

  ctx.token_sequence.push_back({id, val});
}
} // namespace

namespace cfg {
std::vector<token_t> tokenize(const lexer_table_t *tbl,
                              const lexer_input_t *inp) {
  lexer_context ctx{};

  for (std::size_t i = 0; i < inp->size(); ++i) {
		ctx.mode = detect_mode((*inp)[i]);

		if (handle_single_id(tbl, (*inp)[i], ctx) == result::success)
			continue;

                if (handle_id_list(tbl, (*inp)[i], ctx) == result::success)
                  continue;

		if (ctx.mode == lexer_mode::forced_non_id)
			++i;

    if (i < inp->size())
      handle_non_id(tbl, (*inp)[i], ctx);
  }

  return std::move(ctx.token_sequence);
}
} // namespace cfg

namespace {
cfg::lexer_mode detect_mode(const std::string &token) {
  if (token.size() < 2)
    return cfg::lexer_mode::non_id;

  if (token[0] == '-' && token[1] == '-') {
    if (token.size() == 2)
      return cfg::lexer_mode::forced_non_id;
    else
      return cfg::lexer_mode::single_id;
  }

  if (token[0] == '-' && token[1] != '-' && token[1] != ' ') {
    if (token.size() == 2)
      return cfg::lexer_mode::single_id;
    return cfg::lexer_mode::id_list;
  }

  return cfg::lexer_mode::non_id;
}
} // namespace

namespace {
cfg::result find_table_entries(const cfg::lexer_table_t *table,
                               const std::string &str,
                               cfg::lexer_table_t *findings) {
  cfg::result r{cfg::result::match_failure};

  for (const auto &entry : *table) {
    if (std::regex_match(str, entry.pattern)) {
      if (findings)
        findings->push_back(entry);
      r = cfg::result::success;
    }
  }

  return r;
}

cfg::result first_token_id_match(const cfg::lexer_table_t *tbl,
                                 const cfg::token_type type,
                                 const std::string &str, std::string *id) {
  cfg::lexer_table_t matches{};
  if (auto r = find_table_entries(tbl, str, &matches);
      r != cfg::result::success)
    return r;

  for (const auto &m : matches) {
    if (m.type == type) {
      if (id)
        *id = m.id;
      return cfg::result::success;
    }
  }

  return cfg::result::match_failure;
}
} // namespace

namespace {
cfg::result tokenize_single_id(const cfg::lexer_table_t *tbl,
                               const std::string &str, cfg::token_t *tok) {
  std::string id{};
  auto r = first_token_id_match(tbl, cfg::token_type::option, str, &id);
  if (r != cfg::result::success)
    r = first_token_id_match(tbl, cfg::token_type::flag, str, &id);

  if (tok) {
    tok->id = std::move(id);
    tok->value = str;
  }

  return r;
}
} // namespace

namespace {
cfg::result tokenize_id_list(const cfg::lexer_table_t *tb,
                             const std::string &fl,
                             std::vector<cfg::token_t> *&tl) {
  std::vector<cfg::token_t> toks{};

  for (std::size_t i = 1; i < fl.size(); ++i) {
    const auto s = std::string{"-"} + fl[i];
    std::string id{};
    const auto opt = first_token_id_match(tb, cfg::token_type::option, s, &id);
    cfg::token_t tok{};
    if (auto r = tokenize_flag(tb, fl[i], &tok); r == cfg::result::success)
      toks.push_back(std::move(tok));
    else if (i == fl.size() - 1 && opt == cfg::result::success) {
      tok.id = std::move(id);
      tok.value = fl[i];
      toks.push_back(std::move(tok));
    } else
      return cfg::result::match_failure;
  }

  if (tl) {
    tl->reserve(tl->size() + toks.size());
    for (auto &&e : toks)
      tl->push_back(std::move(e));
  }

  return cfg::result::success;
}
} // namespace

namespace {
cfg::result tokenize_flag(const cfg::lexer_table_t *tbl, const std::string &str,
                          cfg::token_t *tok) {
  std::string id{};
  auto r = first_token_id_match(tbl, cfg::token_type::flag, str, &id);

  if (tok) {
    tok->id = std::move(id);
    tok->value = str;
  }

  return r;
}

cfg::result tokenize_flag(const cfg::lexer_table_t *l, const char c,
                          cfg::token_t *t) {
  return tokenize_flag(l, std::string{"-"} + c, t);
}
} // namespace

std::ostream &operator<<(std::ostream &o, const cfg::token_type &t) {
  switch (t) {
  case cfg::token_type::option:
    o << "option";
    break;
  case cfg::token_type::flag:
    o << "flag";
    break;
  case cfg::token_type::free:
    o << "free";
    break;
  }
  return o;
}

namespace cfg {
result read_from_file(const std::string &path, lexer_table_t *tbl) {
  std::ifstream str{path};
  if (!str.is_open())
    return result::file_access_failure;

  constexpr std::size_t expected_fields{3};
  constexpr std::size_t type_field{0};
  constexpr std::size_t id_field{1};
  constexpr std::size_t regex_field{2};
  constexpr bool skip_empty{true};
  token_type type{};
  std::string record{};

  while (std::getline(str, record)) {
    auto fields = flt::split<std::list>(record, " ", skip_empty);
    if (fields.size() != expected_fields)
      return result::format_error;

    const auto fv = flt::to_container<std::vector>(fields);
    if (fv[type_field] == "option")
      type = token_type::option;
    else if (fv[type_field] == "flag")
      type = token_type::flag;
    else if (fv[type_field] == "free")
      type = token_type::free;
    else
      return result::bad_arg_type;

    if (tbl)
      add_entry(tbl, type, fv[id_field], fv[regex_field]);
  }
  return result::success;
}

std::string to_string(const lexer_table_t *t) {
  if (!t->size())
    return {};

  std::size_t max{t->front().id.size()};
  for (const auto &entry : *t)
    if (entry.id.size() > max)
      max = entry.id.size();

  std::stringstream out{};
  for (std::size_t i = 0; i < t->size(); ++i) {
    out << (*t)[i].id;
    for (std::size_t j = 0; j < max - (*t)[i].id.size(); ++j)
      out << " ";
    out << " -> " << (*t)[i].type << " " << (*t)[i].regex;
    if (i < t->size() - 1)
      out << '\n';
  }

  return out.str();
}
} // namespace cfg
