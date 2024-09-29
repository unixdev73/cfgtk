module;

#include <utility>
#include <vector>

export module cfgtk_grammar;

export namespace cfg {
template <typename T> struct context {
  using value_t = T;
  using rhs_t = std::vector<value_t>; // rhs = right hand side,
                                      // as in the rhs of a production rule
  using rule_t = std::pair<value_t, rhs_t>;
  using grammar_t = std::vector<rule_t>;
};

template <typename T>
void left_hand_sides(const typename context<T>::rhs_t *const rhs,
                     const typename context<T>::grammar_t *const g,
                     std::vector<T> *const lhs) {
  for (const auto &rule : *g) {
    if (rule.second.size() != rhs->size())
      continue;
    lhs->push_back(rule.first);
    for (decltype(rhs->size()) i = 0; i < rhs->size(); ++i)
      if (rule.second[i] != rhs->at(i)) {
        lhs->pop_back();
        break;
      }
  }
}

template <typename T>
void left_hand_sides(const std::pair<T, T> *const rhs,
                     const typename context<T>::grammar_t *const g,
                     std::vector<T> *const lhs) {
  typename context<T>::rhs_t vec = {rhs->first, rhs->second};
  left_hand_sides<T>(&vec, g, lhs);
}

template <typename T>
std::vector<T> left_hand_sides(const typename context<T>::rhs_t *const rhs,
                               const typename context<T>::grammar_t *const g) {
  std::vector<T> lhs{};
  left_hand_sides(rhs, g, &lhs);
  return lhs;
}

template <typename T>
auto cartesian_product(const std::vector<T> *const a,
                       const std::vector<T> *const b) {
  std::vector<std::pair<T, T>> prod{};

  if (!a || !b)
    return prod;
  if (a->size() && b->size())
    prod.reserve(a->size() * b->size());

  for (decltype(a->size()) i = 0; i < a->size(); ++i)
    for (decltype(b->size()) j = 0; j < b->size(); ++j)
      prod.push_back({a->at(i), b->at(j)});
  return prod;
}
} // namespace cfg
