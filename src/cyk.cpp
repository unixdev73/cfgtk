module;

#include <iostream>
#include <vector>

import cfgtk_grammar;
export module cfgtk_cyk;

export namespace cyk {
template <typename T>
using recognition_table = std::vector<std::vector<std::vector<T>>>;

template <typename T>
bool init_rtable(recognition_table<T> *const rt,
                 const std::vector<T> *const input,
                 const typename cfg::context<T>::grammar_t *const g) {
  for (decltype(input->size()) i = 0; i < input->size(); ++i) {
    typename cfg::context<T>::rhs_t rhs = {input->at(i)};
    auto &row0 = (*rt)[0][i];
    row0 = cfg::left_hand_sides<T>(&rhs, g);
    if (!row0.size())
      return false;
  }
  return true;
}

template <typename T>
bool parse(const std::vector<T> *const input,
           const typename cfg::context<T>::grammar_t *const g) {
  if (!input || !g || !input->size() || !g->size())
    return false;

  recognition_table<T> rtable(
      input->size(),
      std::vector<std::vector<T>>(input->size(), std::vector<T>{}));

  if (!init_rtable(&rtable, input, g))
    return false;

  auto width = input->size() - 1;

  for (decltype(input->size()) row = 1; row < input->size(); ++row) {
    for (decltype(width) col = 0; col < width; ++col) {
      for (decltype(row) i = 0; i < row; ++i) {
        const auto *const vert_cell = &rtable[i][col];
        const auto *const diag_cell = &rtable[row - i - 1][col + i + 1];

        if (vert_cell->empty() || diag_cell->empty())
          continue;

        auto product = cfg::cartesian_product(vert_cell, diag_cell);
        for (const auto &p : product)
          cfg::left_hand_sides(&p, g, &rtable[row][col]);
      }
    }
    --width;
  }

  for (const auto &row : rtable) {
    std::cout << "| ";
    for (const auto &cell : row) {
      for (decltype(cell.size()) i = 0; i < cell.size(); ++i) {
        std::cout << cell[i];
        if (i != cell.size() - 1)
          std::cout << ", ";
      }
      std::cout << " | ";
    }
    std::cout << std::endl;
  }

  for (const auto &nterm : rtable[input->size() - 1][0])
    if (nterm == (*g)[0].first)
      return true;
  return false;
}
} // namespace cyk
