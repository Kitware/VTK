// https://www.reedbeta.com/blog/python-like-enumerate-in-cpp17/
// std::vector<Thing> things;
// ...
// for (auto [i, thing] : enumerate(things))
// {
//     .. `i` gets the index and `thing` gets the Thing in each iteration
// }

#include <tuple>

#include "vtk_ioss_mangle.h"

namespace Ioss {
  template <typename T, typename TIter = decltype(std::begin(std::declval<T>())),
    typename value_type = typename std::iterator_traits<TIter>::value_type,
    typename = decltype(std::end(std::declval<T>()))>
  struct iterator
  {
    size_t i;
    TIter iter;
    bool operator!=(const iterator& other) const { return iter != other.iter; }
    void operator++()
    {
      ++i;
      ++iter;
    }
    auto operator*() -> std::tuple<size_t, value_type> const { return std::tie(i, *iter); }
  };
  template <typename T>
  struct iterable_wrapper
  {
    T iterable;
    auto begin() -> iterator<T> { return iterator<T>{ 0, std::begin(iterable) }; }
    auto end() -> iterator<T> { return iterator<T>{ 0, std::end(iterable) }; }
  };

  template <typename T>
  constexpr auto enumerate(T &&iterable) -> iterable_wrapper<T>
  {
    return iterable_wrapper<T>{std::forward<T>(iterable)};
  }
} // namespace Ioss
