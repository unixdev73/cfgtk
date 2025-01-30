#include <cfgtk/lexer.hpp>
#include <cfgtk/parser.hpp>
#include <cmath>
#include <iostream>

int main(int argc, char **argv) {
  if (argc > 1 && argc != 3) {
    std::cerr << "Error: wrong amount of parameters;"
                 " Usage: <expected_output> '<input>'"
              << std::endl;
    return 1;
  }

  using cfg::add_rule;
  using cfg::bind;

  const double expected{argc > 1 ? std::stod(argv[1]) : 0};
  const bool skip_space{true};

  cfg::text_encoding enc{}; // Specifies format when converting to string
  cfg::lexer_table_t tbl{};
  cfg::action_map_t m{};
  cfg::grammar_t g{};

  bool exit_prog{false}, v{false}; // v - verbose
  double result{};

  // First, we define how the input should be tokenized.
  // This involves specifying the token type,
  // which determines how the lexer processes the input.
  // For this calculator, all tokens are non-CLI tokens, or "free" tokens.
  // Next, we specify the token ID that will be returned
  // when a match is found based on the regular expression
  // in the third field.
  cfg::add_entry(&tbl, cfg::token_type::free, "quit", "quit|q");
  cfg::add_entry(&tbl, cfg::token_type::free, "plus", "\\+");
  cfg::add_entry(&tbl, cfg::token_type::free, "minus", "-");
  cfg::add_entry(&tbl, cfg::token_type::free, "star", "\\*");
  cfg::add_entry(&tbl, cfg::token_type::free, "slash", "/");
  cfg::add_entry(&tbl, cfg::token_type::free, "caret", "\\^");
  cfg::add_entry(&tbl, cfg::token_type::free, "open", "\\(");
  cfg::add_entry(&tbl, cfg::token_type::free, "close", "\\)");
  cfg::add_entry(&tbl, cfg::token_type::free, "dot", "\\.");
  cfg::add_entry(&tbl, cfg::token_type::free, "zero", "0");
  cfg::add_entry(&tbl, cfg::token_type::free, "positive", "[1-9]");

  /* ORIGINAL PRE-CNF CONVERSION GRAMMAR:
  {
    cfg::grammar_t o{};
                add_rule(&o, "expr", "quit");
                add_rule(&o, "expr", "term");
                add_rule(&o, "expr", "expr", "plus", "term");
                add_rule(&o, "expr", "expr", "minus", "term");
                add_rule(&o, "term", "factor");
                add_rule(&o, "term", "term", "star", "factor");
                add_rule(&o, "term", "term", "slash", "factor");
                add_rule(&o, "factor", "primary");
                add_rule(&o, "factor", "factor", "caret", "primary");
                add_rule(&o, "primary", "number");
                add_rule(&o, "primary", "open", "minus", "number", "close");
                add_rule(&o, "primary", "open", "expr", "close");
                add_rule(&o, "number", "integer");
                add_rule(&o, "number", "float");
                add_rule(&o, "integer", "digit");
                add_rule(&o, "integer", "positive", "integer");
                add_rule(&o, "float", "digit", "dot", "integer");
                add_rule(&o, "digit", "zero");
                add_rule(&o, "digit", "positive");

    cfg::cnf_info conf{};
    conf.filter = cfg::cnf_filter::unique0 | cfg::cnf_filter::start |
                  cfg::cnf_filter::term | cfg::cnf_filter::bin |
                  cfg::cnf_filter::del | cfg::cnf_filter::unique1 |
                  cfg::cnf_filter::unit | cfg::cnf_filter::unique2 |
                                                                        cfg::cnf_filter::prune
  | cfg::cnf_filter::group;

                // This grammar is not suitable for CYK parsing, so we use the
  following
                // function to convert our CFG into CNF. Once the conversion is
  complete,
                // we embed the generated code and attach the corresponding
  semantic actions. cfg::to_cnf(&o, &g, &conf); cfg::cpp_encoding e{};
                e.action_map_name = "m";
                e.action_name = "f";
    e.grammar_name = "g";
                e.namespace_on = false;

    const auto cnf_grammar = cfg::to_string(&g, &e) + "\n";
    std::cout << "CNF GRAMMAR (" << g.size() << " entries): \n" << cnf_grammar;
                std::cout << std::endl;
  }
  return 0;
        */

  // This callback signals that the user requested to exit the program
  const cfg::action_t q = [&exit_prog](auto *, auto *, auto *) {
    std::cout << "Have a nice day." << std::endl;
    exit_prog = true;
  };

  // This instructs the parser to assign the value of the left (or head)
  // RHS symbol to the matched rule.
  const cfg::action_t assignl = [&v](auto *lhs, auto *left, auto *) {
    if (v) {
      std::cout << lhs->rule.entry->lhs << " = ";
      std::cout << left->value << "\n";
    }
    lhs->value = left->value;
  };

  // This instructs the parser to assign the value of the right (or tail)
  // RHS symbol to the matched rule.
  const cfg::action_t assignr = [&v](auto *lhs, auto *, auto *right) {
    if (v) {
      std::cout << lhs->rule.entry->lhs << " = ";
      std::cout << right->value << "\n";
    }
    lhs->value = right->value;
  };

  // This instructs the parser to assign the concatenated value of the
  // left (or head) and right (or tail) RHS symbols to the matched rule.
  const cfg::action_t merge = [&v](auto *lhs, auto *left, auto *right) {
    lhs->value = left->value + right->value;
    if (v) {
      std::cout << lhs->rule.entry->lhs << " = ";
      std::cout << lhs->value << std::endl;
    }
  };

  // This one is used to update the variable that stores the final result
  const cfg::action_t update = [&result, &v](auto *lhs, auto *, auto *) {
    if (v)
      std::cout << "updating result" << std::endl;
    result = std::stof(lhs->value);
  };

  // The following callbacks execute the corresponding arithmetic operations.
  const cfg::action_t add = [&v](auto *lhs, auto *left, auto *right) {
    lhs->value =
        std::to_string(std::stof(left->value) + std::stof(right->value));
    if (v) {
      std::cout << lhs->rule.entry->lhs << " = ";
      std::cout << lhs->value << "\n";
    }
  };

  const cfg::action_t subtract = [&v](auto *lhs, auto *left, auto *right) {
    lhs->value =
        std::to_string(std::stof(left->value) - std::stof(right->value));
    if (v) {
      std::cout << lhs->rule.entry->lhs << " = ";
      std::cout << lhs->value << "\n";
    }
  };

  const cfg::action_t multiply = [&v](auto *lhs, auto *left, auto *right) {
    lhs->value =
        std::to_string(std::stof(left->value) * std::stof(right->value));
    if (v) {
      std::cout << lhs->rule.entry->lhs << " = ";
      std::cout << lhs->value << "\n";
    }
  };

  const cfg::action_t divide = [&v](auto *lhs, auto *left, auto *right) {
    lhs->value =
        std::to_string(std::stof(left->value) / std::stof(right->value));
    if (v) {
      std::cout << lhs->rule.entry->lhs << " = ";
      std::cout << lhs->value << "\n";
    }
  };

  const cfg::action_t power = [&v](auto *lhs, auto *left, auto *right) {
    lhs->value = std::to_string(
        std::pow(std::stof(left->value), std::stof(right->value)));
    if (v) {
      std::cout << lhs->rule.entry->lhs << " = ";
      std::cout << lhs->value << "\n";
    }
  };

  // The add_rule function adds a production rule to the grammar and
  // returns a pointer to the newly added rule.
  // The bind function associates a semantic action to be executed
  // when a specific instance of a rule is matched.
  // Note that the start symbol rules update the variable with the final
  // result upon completion.
  auto r0 = add_rule(&g, "expr#0", "expr", "expr#1");
  bind(&m, r0, add);
  bind(&m, r0, update);

  auto r1 = add_rule(&g, "expr#0", "expr", "expr#2");
  bind(&m, r1, subtract);
  bind(&m, r1, update);

  auto r2 = add_rule(&g, "expr#0", "term", "term#0");
  bind(&m, r2, multiply);
  bind(&m, r2, update);

  auto r3 = add_rule(&g, "expr#0", "term", "term#1");
  bind(&m, r3, divide);
  bind(&m, r3, update);

  auto r4 = add_rule(&g, "expr#0", "factor", "factor#0");
  bind(&m, r4, power);
  bind(&m, r4, update);

  auto r5 = add_rule(&g, "expr#0", "open#0", "primary#0");
  bind(&m, r5, assignr);
  bind(&m, r5, update);

  auto r6 = add_rule(&g, "expr#0", "open#1", "primary#2");
  bind(&m, r6, assignr);
  bind(&m, r6, update);

  auto r7 = add_rule(&g, "expr#0", "digit", "float#0");
  bind(&m, r7, merge);
  bind(&m, r7, update);

  auto r8 = add_rule(&g, "expr#0", "positive#0", "integer");
  bind(&m, r8, merge);
  bind(&m, r8, update);

  bind(&m, add_rule(&g, "expr#0", "positive"), update);
  bind(&m, add_rule(&g, "expr#0", "zero"), update);

  bind(&m, add_rule(&g, "expr", "expr", "expr#1"), add);
  bind(&m, add_rule(&g, "expr", "expr", "expr#2"), subtract);
  bind(&m, add_rule(&g, "expr", "term", "term#0"), multiply);
  bind(&m, add_rule(&g, "expr", "term", "term#1"), divide);
  bind(&m, add_rule(&g, "expr", "factor", "factor#0"), power);
  bind(&m, add_rule(&g, "expr", "open#0", "primary#0"), assignr);
  bind(&m, add_rule(&g, "expr", "open#1", "primary#2"), assignr);
  bind(&m, add_rule(&g, "expr", "digit", "float#0"), merge);
  bind(&m, add_rule(&g, "expr", "positive#0", "integer"), merge);
  bind(&m, add_rule(&g, "term", "term", "term#0"), multiply);
  bind(&m, add_rule(&g, "term", "term", "term#1"), divide);
  bind(&m, add_rule(&g, "term", "factor", "factor#0"), power);
  bind(&m, add_rule(&g, "term", "open#0", "primary#0"), assignr);
  bind(&m, add_rule(&g, "term", "open#1", "primary#2"), assignr);
  bind(&m, add_rule(&g, "term", "digit", "float#0"), merge);
  bind(&m, add_rule(&g, "term", "positive#0", "integer"), merge);
  bind(&m, add_rule(&g, "factor", "factor", "factor#0"), power);
  bind(&m, add_rule(&g, "factor", "open#0", "primary#0"), assignr);
  bind(&m, add_rule(&g, "factor", "open#1", "primary#2"), assignr);
  bind(&m, add_rule(&g, "factor", "digit", "float#0"), merge);
  bind(&m, add_rule(&g, "factor", "positive#0", "integer"), merge);
  bind(&m, add_rule(&g, "primary", "open#0", "primary#0"), assignr);
  bind(&m, add_rule(&g, "primary", "open#1", "primary#2"), assignr);
  bind(&m, add_rule(&g, "primary", "digit", "float#0"), merge);
  bind(&m, add_rule(&g, "primary", "positive#0", "integer"), merge);
  bind(&m, add_rule(&g, "number", "digit", "float#0"), merge);
  bind(&m, add_rule(&g, "number", "positive#0", "integer"), merge);
  bind(&m, add_rule(&g, "integer", "positive#0", "integer"), merge);
  bind(&m, add_rule(&g, "float", "digit", "float#0"), merge);
  bind(&m, add_rule(&g, "expr#1", "plus#0", "term"), assignr);
  bind(&m, add_rule(&g, "expr#2", "minus#0", "term"), assignr);
  bind(&m, add_rule(&g, "term#0", "star#0", "factor"), assignr);
  bind(&m, add_rule(&g, "term#1", "slash#0", "factor"), assignr);
  bind(&m, add_rule(&g, "factor#0", "caret#0", "primary"), assignr);
  bind(&m, add_rule(&g, "primary#0", "minus#1", "primary#1"), merge);
  bind(&m, add_rule(&g, "primary#1", "number", "close#0"), assignl);
  bind(&m, add_rule(&g, "primary#2", "expr", "close#1"), assignl);
  bind(&m, add_rule(&g, "float#0", "dot#0", "integer"), merge);

  add_rule(&g, "expr", "zero");
  add_rule(&g, "expr", "positive");
  add_rule(&g, "term", "zero");
  add_rule(&g, "term", "positive");
  add_rule(&g, "factor", "zero");
  add_rule(&g, "factor", "positive");
  add_rule(&g, "primary", "zero");
  add_rule(&g, "primary", "positive");
  add_rule(&g, "number", "zero");
  add_rule(&g, "number", "positive");
  add_rule(&g, "integer", "zero");
  add_rule(&g, "integer", "positive");
  add_rule(&g, "digit", "zero");
  add_rule(&g, "digit", "positive");
  add_rule(&g, "plus#0", "plus");
  add_rule(&g, "minus#0", "minus");
  add_rule(&g, "star#0", "star");
  add_rule(&g, "slash#0", "slash");
  add_rule(&g, "caret#0", "caret");
  add_rule(&g, "open#0", "open");
  add_rule(&g, "minus#1", "minus");
  add_rule(&g, "close#0", "close");
  add_rule(&g, "open#1", "open");
  add_rule(&g, "close#1", "close");
  add_rule(&g, "positive#0", "positive");
  add_rule(&g, "dot#0", "dot");
  bind(&m, add_rule(&g, "expr#0", "quit"), q);

  while (!exit_prog) {
    std::string cli{};
    result = 0;

    if (argc > 1)
      cli = argv[2];
    else {
      std::cout << "calc> ";
      std::getline(std::cin, cli);
    }

    // This utility function splits the input string,
    // omitting any empty spaces in between.
    const auto input = flt::split<std::vector, std::string>(cli, skip_space);

    // This function invokes the lexer, returning tokens that include
    // the IDs from the table above, along with the specific values
    // matching the regex of the corresponding entries.
    const auto tokens = cfg::tokenize(&tbl, &input);

    // Finally, we use our grammar, tokens, and callback_map to validate
    // the sequence according to the grammar. For each parse tree, a sequence
    // of callbacks is generated, allowing their execution
    // to be deferred to a later time.
    const auto chart = cfg::cyk(&g, &tokens, &m);

    if (!cfg::is_valid(&chart, cfg::get_start(&g))) {
      std::cerr << "Done parsing; status: NOK\n" << std::endl;
      if (tokens.size() <= 7)
        std::cout << cfg::to_string(&chart) << "\n" << std::endl;
      if (argc > 1)
        return 2;
      continue;
    }

    const auto trees = cfg::get_trees(&chart, cfg::get_start(&g));
    std::cout << "Done parsing; status: OK\n";

    // This function executes the semantic actions.
    // It is useful when a reaction is needed after the parse stage.
    cfg::run_actions(&trees.front(), &m);
    if (exit_prog)
      return 0;

    std::cout << "\n" << cfg::to_string(&trees.front(), &tokens) << "\n\n";
    std::cout << "Result: " << result << std::endl;

    if (argc > 1) {
      if (result == expected)
        return 0;
      else
        return 3;
    }
  }
}
