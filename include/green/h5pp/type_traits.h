//
// Created by iskakoff on 9/14/23.
//

#ifndef H5PP_TYPE_TRAITS_H
#define H5PP_TYPE_TRAITS_H

#include <complex>
#include <type_traits>

namespace green::h5pp {
  template <typename T>
  struct is_complex_t : std::false_type {};
  template <typename T>
  struct is_complex_t<std::complex<T>> : std::true_type {};
  template <typename T>
  constexpr bool is_complex_scalar = is_complex_t<T>::value;
  template <typename T>
  constexpr bool is_scalar =
      std::is_arithmetic_v<T> || std::is_same_v<T, std::string> ||
      std::is_arithmetic_v<std::remove_reference_t<T>> || is_complex_scalar<std::remove_reference_t<T>>;

  template <typename... Ts>
  using void_t = void;
  template <template <class...> class Trait, class AlwaysVoid, class... Args>
  struct detector : std::false_type {};
  template <template <class...> class Trait, class... Args>
  struct detector<Trait, void_t<Trait<Args...>>, Args...> : std::true_type {};

  // size_method_t is a detector type for T.size() const
  template <typename T>
  using size_method_t = decltype(std::declval<const T&>().size());
  // shape_method_t is a detector type for T.shape() const
  template <typename T>
  using shape_method_t = decltype(std::declval<const T&>().shape());
  // reshape_method_t is a detector type for T.reshape() const
  template <typename T>
  using reshape_method_t = decltype(std::declval<T&>().reshape(std::declval<std::vector<std::size_t>>()));
  // reshape_method_t is a detector type for T.reshape() const
  template <typename T>
  using resize_method_t = decltype(std::declval<T&>().resize(std::declval<std::size_t>()));
  // data_method_t is a detector type for T.data() const
  template <typename T>
  using data_method_t = decltype(std::declval<T&>().data());

  // Check if data container has certain methods
  template <typename T>
  using has_data_t = typename detector<data_method_t, void, T>::type;
  template <typename T>
  using has_size_t = typename detector<size_method_t, void, T>::type;
  template <typename T>
  using has_shape_t = typename detector<shape_method_t, void, T>::type;
  template <typename T>
  using has_resize_t = typename detector<resize_method_t, void, T>::type;
  template <typename T>
  using has_reshape_t = typename detector<reshape_method_t, void, T>::type;

  template <typename T>
  constexpr bool is_1D_array = has_data_t<T>::value && has_size_t<T>::value && !has_shape_t<T>::value;
  template <typename T>
  constexpr bool is_ND_array = has_data_t<T>::value && has_size_t<T>::value && has_shape_t<T>::value;
  template <typename T>
  constexpr bool is_resizable = has_resize_t<T>::value;
  template <typename T>
  constexpr bool is_reshapable = has_reshape_t<T>::value;

}  // namespace green::h5pp

#endif  // H5PP_TYPE_TRAITS_H
