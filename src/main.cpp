#include <iostream>
#include <string>
#include <vector>

import cfgtk_grammar;
import cfgtk_cyk;

int main() {
  typename cfg::context<std::string>::grammar_t g = {
      {"S", {"A", "B"}}, {"A", {"C", "D"}}, {"A", {"C", "F"}},
      {"B", {"c"}},      {"B", {"E", "B"}}, {"C", {"a"}},
      {"D", {"b"}},      {"E", {"c"}},      {"F", {"A", "D"}}};

  std::vector<std::string> word = {"a", "a", "a", "b", "b", "b", "c", "c"};

  if (cyk::parse(&word, &g)) {
    for (const auto &l : word)
      std::cout << l;
    std::cout << " is a valid construct." << std::endl;
  } else {
    for (const auto &l : word)
      std::cout << l;
    std::cout << " is not a valid construct." << std::endl;
  }
}
