//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#if !defined(VISKORES_DEVICE) || !defined(VISKORES_NAMESPACE)
#error VariantImpl.h must be included from Variant.h
// Some defines to make my IDE happy.
#define VISKORES_DEVICE
#define VISKORES_NAMESPACE tmp
#endif

#include <viskores/internal/VariantImplDetail.h>

#include <viskores/List.h>

#include <viskores/internal/Assume.h>

namespace viskores
{
namespace VISKORES_NAMESPACE
{

// Forward declaration
template <typename... Ts>
class Variant;

namespace detail
{

// --------------------------------------------------------------------------------
// Helper classes for Variant

template <typename UnionType>
struct VariantUnionToListImpl;
template <typename... Ts>
struct VariantUnionToListImpl<detail::VariantUnionTD<Ts...>>
{
  using type = viskores::List<Ts...>;
};
template <typename... Ts>
struct VariantUnionToListImpl<detail::VariantUnionNTD<Ts...>>
{
  using type = viskores::List<Ts...>;
};

template <typename UnionType>
using VariantUnionToList =
  typename VariantUnionToListImpl<typename std::decay<UnionType>::type>::type;

struct VariantCopyConstructFunctor
{
  template <typename T, typename UnionType>
  VISKORES_DEVICE void operator()(const T& src, UnionType& destUnion) const noexcept
  {
    constexpr viskores::IdComponent Index =
      viskores::ListIndexOf<VariantUnionToList<UnionType>, T>::value;
    // If we are using this functor, we can assume the union does not hold a valid type.
    new (&VariantUnionGet<Index>(destUnion)) T(src);
  }
};

struct VariantCopyFunctor
{
  template <typename T, typename UnionType>
  VISKORES_DEVICE void operator()(const T& src, UnionType& destUnion) const noexcept
  {
    constexpr viskores::IdComponent Index =
      viskores::ListIndexOf<VariantUnionToList<UnionType>, T>::value;
    // If we are using this functor, we can assume the union holds type T.
    this->DoCopy(
      src, VariantUnionGet<Index>(destUnion), typename std::is_copy_assignable<T>::type{});
  }

  template <typename T>
  VISKORES_DEVICE void DoCopy(const T& src, T& dest, std::true_type) const noexcept
  {
    dest = src;
  }

  template <typename T>
  VISKORES_DEVICE void DoCopy(const T& src, T& dest, std::false_type) const noexcept
  {
    if (&src != &dest)
    {
      // Do not have an assignment operator, so destroy the old object and create a new one.
      dest.~T();
      new (&dest) T(src);
    }
    else
    {
      // Objects are already the same.
    }
  }
};

struct VariantDestroyFunctor
{
  template <typename T>
  VISKORES_DEVICE void operator()(T& src) const noexcept
  {
    src.~T();
  }
};

template <typename T>
struct VariantCheckType
{
  // We are currently not allowing reference types (e.g. FooType&) or pointer types (e.g. FooType*)
  // in Variant objects. References and pointers can fail badly when things are passed around
  // devices. If you get a compiler error here, consider removing the reference or using something
  // like std::decay to remove qualifiers on the type. (We may decide to do that automatically in
  // the future.)
  VISKORES_STATIC_ASSERT_MSG(!std::is_reference<T>::value,
                             "References are not allowed in Viskores Variant.");
  VISKORES_STATIC_ASSERT_MSG(!std::is_pointer<T>::value,
                             "Pointers are not allowed in Viskores Variant.");
};

template <typename VariantType>
struct VariantTriviallyCopyable;

template <typename... Ts>
struct VariantTriviallyCopyable<viskores::VISKORES_NAMESPACE::Variant<Ts...>>
  : AllTriviallyCopyable<Ts...>
{
};

template <typename VariantType>
struct VariantTriviallyConstructible;

template <typename... Ts>
struct VariantTriviallyConstructible<viskores::VISKORES_NAMESPACE::Variant<Ts...>>
  : AllTriviallyConstructible<Ts...>
{
};

// --------------------------------------------------------------------------------
// Variant superclass that defines its storage
template <typename... Ts>
struct VariantStorageImpl
{
  VariantUnion<Ts...> Storage;
  viskores::IdComponent Index;

  VariantStorageImpl() = default;

  VISKORES_DEVICE VariantStorageImpl(viskores::internal::NullType dummy)
    : Storage({ dummy })
  {
  }

  template <viskores::IdComponent Index>
  using TypeAt = typename viskores::ListAt<viskores::List<Ts...>, Index>;

  VISKORES_DEVICE viskores::IdComponent GetIndex() const noexcept { return this->Index; }
  VISKORES_DEVICE bool IsValid() const noexcept
  {
    return (this->Index >= 0) && (this->Index < static_cast<viskores::IdComponent>(sizeof...(Ts)));
  }

  VISKORES_DEVICE void Reset() noexcept
  {
    if (this->IsValid())
    {
      this->CastAndCall(detail::VariantDestroyFunctor{});
      this->Index = -1;
    }
  }

  template <typename Functor, typename... Args>
  VISKORES_DEVICE auto CastAndCall(Functor&& f, Args&&... args) const
    noexcept(noexcept(f(std::declval<const TypeAt<0>&>(), args...)))
      -> decltype(f(std::declval<const TypeAt<0>&>(), args...))
  {
    VISKORES_ASSERT(this->IsValid());
    return detail::VariantCastAndCallImpl<sizeof...(Ts)>(
      this->GetIndex(), std::forward<Functor>(f), this->Storage, std::forward<Args>(args)...);
  }

  template <typename Functor, typename... Args>
  VISKORES_DEVICE auto CastAndCall(Functor&& f, Args&&... args) noexcept(
    noexcept(f(std::declval<const TypeAt<0>&>(), args...)))
    -> decltype(f(std::declval<TypeAt<0>&>(), args...))
  {
    VISKORES_ASSERT(this->IsValid());
    return detail::VariantCastAndCallImpl<sizeof...(Ts)>(
      this->GetIndex(), std::forward<Functor>(f), this->Storage, std::forward<Args>(args)...);
  }
};

// --------------------------------------------------------------------------------
// Variant superclass that helps preserve trivially copyable and trivially constructable
// properties where possible.
template <typename VariantType,
          typename TriviallyConstructible =
            typename VariantTriviallyConstructible<VariantType>::type,
          typename TriviallyCopyable = typename VariantTriviallyCopyable<VariantType>::type>
struct VariantConstructorImpl;

// Can trivially construct, deconstruct, and copy all data. (Probably all trivial classes.)
template <typename... Ts>
struct VariantConstructorImpl<viskores::VISKORES_NAMESPACE::Variant<Ts...>,
                              std::true_type,
                              std::true_type> : VariantStorageImpl<Ts...>
{
  VariantConstructorImpl() = default;
  ~VariantConstructorImpl() = default;

  VariantConstructorImpl(const VariantConstructorImpl&) = default;
  VariantConstructorImpl(VariantConstructorImpl&&) = default;
  VariantConstructorImpl& operator=(const VariantConstructorImpl&) = default;
  VariantConstructorImpl& operator=(VariantConstructorImpl&&) = default;
};

// Can trivially copy, but cannot trivially construct. Common if a class is simple but
// initializes itself.
template <typename... Ts>
struct VariantConstructorImpl<viskores::VISKORES_NAMESPACE::Variant<Ts...>,
                              std::false_type,
                              std::true_type> : VariantStorageImpl<Ts...>
{
  VISKORES_DEVICE VariantConstructorImpl()
    : VariantStorageImpl<Ts...>(viskores::internal::NullType{})
  {
    this->Index = -1;
  }

  // Any trivially copyable class is trivially destructable.
  ~VariantConstructorImpl() = default;

  VariantConstructorImpl(const VariantConstructorImpl&) = default;
  VariantConstructorImpl(VariantConstructorImpl&&) = default;
  VariantConstructorImpl& operator=(const VariantConstructorImpl&) = default;
  VariantConstructorImpl& operator=(VariantConstructorImpl&&) = default;
};

// Cannot trivially copy. We assume we cannot trivially construct/destruct.
template <typename construct_type, typename... Ts>
struct VariantConstructorImpl<viskores::VISKORES_NAMESPACE::Variant<Ts...>,
                              construct_type,
                              std::false_type> : VariantStorageImpl<Ts...>
{
  VISKORES_DEVICE VariantConstructorImpl()
    : VariantStorageImpl<Ts...>(viskores::internal::NullType{})
  {
    this->Index = -1;
  }
  VISKORES_DEVICE ~VariantConstructorImpl() { this->Reset(); }

  VISKORES_DEVICE VariantConstructorImpl(const VariantConstructorImpl& src) noexcept
    : VariantStorageImpl<Ts...>(viskores::internal::NullType{})
  {
    if (src.IsValid())
    {
      src.CastAndCall(VariantCopyConstructFunctor{}, this->Storage);
    }
    this->Index = src.Index;
  }

  VISKORES_DEVICE VariantConstructorImpl& operator=(const VariantConstructorImpl& src) noexcept
  {
    if (src.IsValid())
    {
      if (this->GetIndex() == src.GetIndex())
      {
        src.CastAndCall(detail::VariantCopyFunctor{}, this->Storage);
      }
      else
      {
        this->Reset();
        src.CastAndCall(detail::VariantCopyConstructFunctor{}, this->Storage);
        this->Index = src.Index;
      }
    }
    else
    {
      this->Reset();
    }
    return *this;
  }
};

} // namespace detail

template <typename... Ts>
class Variant : detail::VariantConstructorImpl<Variant<Ts...>>
{
  using Superclass = detail::VariantConstructorImpl<Variant<Ts...>>;

  // Type not used, but has the compiler check all the types for validity.
  using CheckTypes = viskores::List<detail::VariantCheckType<Ts>...>;

public:
  /// Type that converts to a std::integral_constant containing the index of the given type (or
  /// -1 if that type is not in the list).
  template <typename T>
  using IndexOf = viskores::ListIndexOf<viskores::List<Ts...>, T>;

  /// Returns the index for the given type (or -1 if that type is not in the list).
  ///
  template <typename T>
  VISKORES_DEVICE static constexpr viskores::IdComponent GetIndexOf()
  {
    return IndexOf<T>::value;
  }

  /// Type that converts to the type at the given index.
  ///
  template <viskores::IdComponent Index>
  using TypeAt = typename viskores::ListAt<viskores::List<Ts...>, Index>;

  /// \brief Type that indicates whether another type can be stored in this Variant.
  ///
  /// If this templated type resolves to `std::true_type`, then the provided `T` can be
  /// represented in this `Variant`. Otherwise, the type resolves to `std::false_type`.
  ///
  template <typename T>
  using CanStore = std::integral_constant<bool, (IndexOf<T>::value >= 0)>;

  /// Returns whether the given type can be respresented in this Variant.
  ///
  template <typename T>
  VISKORES_DEVICE static constexpr bool GetCanStore()
  {
    return CanStore<T>::value;
  }

  /// The number of types representable by this Variant.
  ///
  static constexpr viskores::IdComponent NumberOfTypes = viskores::IdComponent{ sizeof...(Ts) };

  /// Returns the index of the type of object this variant is storing. If no object is currently
  /// stored (i.e. the `Variant` is invalid), an invalid is returned.
  ///
  VISKORES_DEVICE viskores::IdComponent GetIndex() const noexcept { return this->Index; }

  /// Returns true if this `Variant` is storing an object from one of the types in the template
  /// list, false otherwise.
  ///
  /// Note that if this `Variant` was not initialized with an object, the result of `IsValid`
  /// is undefined. The `Variant` could report itself as validly containing an object that
  /// is trivially constructed.
  ///
  VISKORES_DEVICE bool IsValid() const noexcept
  {
    return (this->Index >= 0) && (this->Index < NumberOfTypes);
  }

  /// Returns true if this `Variant` stores the given type
  ///
  template <typename T>
  VISKORES_DEVICE bool IsType() const
  {
    return (this->GetIndex() == this->GetIndexOf<T>());
  }

  Variant() = default;
  ~Variant() = default;
  Variant(const Variant&) = default;
  Variant(Variant&&) = default;
  Variant& operator=(const Variant&) = default;
  Variant& operator=(Variant&&) = default;

  template <typename T>
  VISKORES_DEVICE Variant(const T& src) noexcept
  {
    constexpr viskores::IdComponent index = GetIndexOf<T>();
    // Might be a way to use an enable_if to enforce a proper type.
    VISKORES_STATIC_ASSERT_MSG(index >= 0, "Attempting to put invalid type into a Variant");

    this->Index = index;
    new (&this->Get<index>()) T(src);
  }

  template <typename T>
  VISKORES_DEVICE Variant& operator=(const T& src)
  {
    if (this->IsType<T>())
    {
      this->Get<T>() = src;
    }
    else
    {
      this->Emplace<T>(src);
    }
    return *this;
  }

  template <typename T, typename... Args>
  VISKORES_DEVICE T& Emplace(Args&&... args)
  {
    constexpr viskores::IdComponent I = GetIndexOf<T>();
    VISKORES_STATIC_ASSERT_MSG(I >= 0, "Variant::Emplace called with invalid type.");
    return this->EmplaceImpl<T, I>(std::forward<Args>(args)...);
  }

  template <typename T, typename U, typename... Args>
  VISKORES_DEVICE T& Emplace(std::initializer_list<U> il, Args&&... args)
  {
    constexpr viskores::IdComponent I = GetIndexOf<T>();
    VISKORES_STATIC_ASSERT_MSG(I >= 0, "Variant::Emplace called with invalid type.");
    return this->EmplaceImpl<T, I>(il, std::forward<Args>(args)...);
  }

  template <viskores::IdComponent I, typename... Args>
  VISKORES_DEVICE TypeAt<I>& Emplace(Args&&... args)
  {
    VISKORES_STATIC_ASSERT_MSG((I >= 0) && (I < NumberOfTypes),
                               "Variant::Emplace called with invalid index");
    return this->EmplaceImpl<TypeAt<I>, I>(std::forward<Args>(args)...);
  }

  template <viskores::IdComponent I, typename U, typename... Args>
  VISKORES_DEVICE TypeAt<I>& Emplace(std::initializer_list<U> il, Args&&... args)
  {
    VISKORES_STATIC_ASSERT_MSG((I >= 0) && (I < NumberOfTypes),
                               "Variant::Emplace called with invalid index");
    return this->EmplaceImpl<TypeAt<I>, I>(il, std::forward<Args>(args)...);
  }

private:
  template <typename T, viskores::IdComponent I, typename... Args>
  VISKORES_DEVICE T& EmplaceImpl(Args&&... args)
  {
    this->Reset();
    this->Index = I;
    return *(new (&this->Get<I>()) T{ args... });
  }

  template <typename T, viskores::IdComponent I, typename U, typename... Args>
  VISKORES_DEVICE T& EmplaceImpl(std::initializer_list<U> il, Args&&... args)
  {
    this->Reset();
    this->Index = I;
    return *(new (&this->Get<I>()) T(il, args...));
  }

public:
  ///@{
  /// Returns the value as the type at the given index. The behavior is undefined if the
  /// variant does not contain the value at the given index.
  ///
  template <viskores::IdComponent I>
  VISKORES_DEVICE TypeAt<I>& Get() noexcept
  {
    VISKORES_ASSERT(I == this->GetIndex());
    return detail::VariantUnionGet<I>(this->Storage);
  }

  template <viskores::IdComponent I>
  VISKORES_DEVICE const TypeAt<I>& Get() const noexcept
  {
    VISKORES_ASSERT(I == this->GetIndex());
    return detail::VariantUnionGet<I>(this->Storage);
  }
  ///@}

  ///@{
  /// Returns the value as the given type. The behavior is undefined if the variant does not
  /// contain a value of the given type.
  ///
  template <typename T>
  VISKORES_DEVICE T& Get() noexcept
  {
    return this->GetImpl<T>(CanStore<T>{});
  }

  template <typename T>
  VISKORES_DEVICE const T& Get() const noexcept
  {
    return this->GetImpl<T>(CanStore<T>{});
  }
  ///@}

private:
  template <typename T>
  VISKORES_DEVICE T& GetImpl(std::true_type)
  {
    VISKORES_ASSERT(this->IsType<T>());
    return detail::VariantUnionGet<IndexOf<T>::value>(this->Storage);
  }

  template <typename T>
  VISKORES_DEVICE const T& GetImpl(std::true_type) const
  {
    VISKORES_ASSERT(this->IsType<T>());
    return detail::VariantUnionGet<IndexOf<T>::value>(this->Storage);
  }

  // This function overload only gets created if you attempt to pull a type from a
  // variant that does not exist. Perhaps this should be a compile error, but there
  // are cases where you might create templated code that has a path that could call
  // this but never does. To make this case easier, do a runtime error (when asserts
  // are active) instead.
  template <typename T>
  VISKORES_DEVICE T& GetImpl(std::false_type) const
  {
    VISKORES_ASSERT(false &&
                    "Attempted to get a type from a variant that the variant does not contain.");
    // This will cause some _really_ nasty issues if you actually try to use the returned type.
    return *reinterpret_cast<T*>(0);
  }

public:
  ///@{
  /// Given a functor object, calls the functor with the contained object cast to the appropriate
  /// type. If extra \c args are given, then those are also passed to the functor after the cast
  /// object. If the functor returns a value, that value is returned from \c CastAndCall.
  ///
  /// The results are undefined if the Variant is not valid.
  ///
  template <typename Functor, typename... Args>
  VISKORES_DEVICE auto CastAndCall(Functor&& f, Args&&... args) const
    noexcept(noexcept(f(std::declval<const TypeAt<0>&>(), args...)))
  {
    VISKORES_ASSERT(this->IsValid());
    return detail::VariantCastAndCallImpl<sizeof...(Ts)>(
      this->GetIndex(), std::forward<Functor>(f), this->Storage, std::forward<Args>(args)...);
  }

  template <typename Functor, typename... Args>
  VISKORES_DEVICE auto CastAndCall(Functor&& f,
                                   Args&&... args) noexcept(noexcept(f(std::declval<TypeAt<0>&>(),
                                                                       args...)))
  {
    VISKORES_ASSERT(this->IsValid());
    return detail::VariantCastAndCallImpl<sizeof...(Ts)>(
      this->GetIndex(), std::forward<Functor>(f), this->Storage, std::forward<Args>(args)...);
  }

  /// Destroys any object the Variant is holding and sets the Variant to an invalid state. This
  /// method is not thread safe.
  ///
  VISKORES_DEVICE void Reset() noexcept
  {
    if (this->IsValid())
    {
      this->CastAndCall(detail::VariantDestroyFunctor{});
      this->Index = -1;
    }
  }
};

/// \brief Convert a `List` to a `Variant`.
///
template <typename List>
using ListAsVariant = viskores::ListApply<List, viskores::VISKORES_NAMESPACE::Variant>;
}
} // namespace viskores::VISKORES_NAMESPACE

#undef VISKORES_DEVICE
#undef VISKORES_NAMESPACE
