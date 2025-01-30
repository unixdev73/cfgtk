#pragma once

#include <algorithm>
#include <cctype>
#include <memory>
#include <ranges>
#include <string>
#include <vector>

namespace flt {
template <typename T> inline T unique(const T &input) {
  T out{};
  for (const auto &e : input)
    if (std::find(out.begin(), out.end(), e) == out.end())
      out.push_back(e);
  return out;
}

template <template <typename> typename C>
C<std::string> to_container(int size, char **data, int begin = 0) {
  C<std::string> container{};
  for (auto index = begin; index < size; ++index)
    container.push_back(std::string{data[index]});
  return container;
}

inline std::string to_container(int size, char **data, int begin) {
  std::string container{};
  for (auto index = begin; index < size; ++index)
    container += std::string{data[index]} + ' ';
  container.pop_back();
  return container;
}

template <template <typename> typename O, template <typename> typename I,
          typename T>
O<std::unique_ptr<T>> to_container(const I<std::unique_ptr<T>> &input) {
  O<std::unique_ptr<T>> o{};
  if constexpr (std::same_as<O<std::unique_ptr<T>>,
                             std::vector<std::unique_ptr<T>>>)
    o.reserve(input.size());

  for (const auto &i : input)
    o.push_back(std::unique_ptr<T>{new T{*i}});
  return o;
}

template <template <typename> typename O, template <typename> typename I,
          typename T>
O<T> to_container(const I<T> &input) {
  O<T> o{};
  if constexpr (std::same_as<O<T>, std::vector<T>>)
    o.reserve(input.size());

  for (const auto &i : input)
    o.push_back(i);
  return o;
}

template <template <typename> typename O, template <typename> typename I,
          typename T>
O<T> to_container(I<T> &&input) {
  O<T> o{};
  if constexpr (std::same_as<O<T>, std::vector<T>>)
    o.reserve(input.size());

  for (auto &&i : input)
    o.push_back(std::move(i));
  return o;
}

template <template <typename> typename C, typename T>
C<T> split(const std::string &input, bool skip_space = false) {
  if (!input.size())
    return {};

  C<T> cont{};
  if constexpr (std::same_as<C<T>, std::vector<T>>)
    cont.reserve(input.size());

  for (const auto &c : input)
    if (!skip_space || !std::isspace(c))
      cont.push_back(T{c});
  return cont;
}

template <template <typename> typename C>
C<std::string> split(const std::string &input, const std::string &d,
                     bool skip_empty = false) {
  if (!input.size())
    return {};

  auto s = std::ranges::views::split(input, d);
  C<std::string> c{};
  if constexpr (std::same_as<C<std::string>, std::vector<std::string>>)
    c.reserve(input.size());

  for (const auto &e : s) {
    auto frag = std::string{e.begin(), e.end()};
    if (!skip_empty || frag.size())
      c.push_back(std::move(frag));
  }
  return c;
}
} // namespace flt
