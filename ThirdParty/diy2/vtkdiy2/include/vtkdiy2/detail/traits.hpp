//--------------------------------------
// utils/traits: Additional type traits
//--------------------------------------
//
//          Copyright kennytm (auraHT Ltd.) 2011.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file doc/LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/**

``<utils/traits.hpp>`` --- Additional type traits
=================================================

This module provides additional type traits and related functions, missing from
the standard library.

*/

#ifndef DIY_UTILS_TRAITS_HPP
#define DIY_UTILS_TRAITS_HPP

#include <cstdlib>
#include <tuple>
#include <functional>
#include <type_traits>

namespace diy
{
namespace detail {

/**
.. macro:: DECLARE_HAS_TYPE_MEMBER(member_name)

    This macro declares a template ``has_member_name`` which will check whether
    a type member ``member_name`` exists in a particular type.

    Example::

        DECLARE_HAS_TYPE_MEMBER(result_type)

        ...

        printf("%d\n", has_result_type< std::plus<int> >::value);
        // ^ prints '1' (true)
        printf("%d\n", has_result_type< double(*)() >::value);
        // ^ prints '0' (false)
*/
#define DECLARE_HAS_TYPE_MEMBER(member_name) \
    template <typename, typename = void> \
    struct has_##member_name \
    { enum { value = false }; }; \
    template <typename T> \
    struct has_##member_name<T, typename std::enable_if<sizeof(typename T::member_name)||true>::type> \
    { enum { value = true }; };

/**
.. type:: struct utils::function_traits<F>

    Obtain compile-time information about a function object *F*.

    This template currently supports the following types:

    * Normal function types (``R(T...)``), function pointers (``R(*)(T...)``)
      and function references (``R(&)(T...)`` and ``R(&&)(T...)``).
    * Member functions (``R(C::*)(T...)``)
    * ``std::function<F>``
    * Type of lambda functions, and any other types that has a unique
      ``operator()``.
    * Type of ``std::mem_fn`` (only for GCC's libstdc++ and LLVM's libc++).
      Following the C++ spec, the first argument will be a raw pointer.
*/
template <typename T>
struct function_traits
    : public function_traits<decltype(&T::operator())>
{};

namespace xx_impl
{
    template <typename C, typename R, typename... A>
    struct memfn_type
    {
        typedef typename std::conditional<
            std::is_const<C>::value,
            typename std::conditional<
                std::is_volatile<C>::value,
                R (C::*)(A...) const volatile,
                R (C::*)(A...) const
            >::type,
            typename std::conditional<
                std::is_volatile<C>::value,
                R (C::*)(A...) volatile,
                R (C::*)(A...)
            >::type
        >::type type;
    };
}

template <typename ReturnType, typename... Args>
struct function_traits<ReturnType(Args...)>
{
    /**
    .. type:: type result_type

        The type returned by calling an instance of the function object type *F*.
    */
    typedef ReturnType result_type;

    /**
    .. type:: type function_type

        The function type (``R(T...)``).
    */
    typedef ReturnType function_type(Args...);

    /**
    .. type:: type member_function_type<OwnerType>

        The member function type for an *OwnerType* (``R(OwnerType::*)(T...)``).
    */
    template <typename OwnerType>
    using member_function_type = typename xx_impl::memfn_type<
        typename std::remove_pointer<typename std::remove_reference<OwnerType>::type>::type,
        ReturnType, Args...
    >::type;

    /**
    .. data:: static const size_t arity

        Number of arguments the function object will take.
    */
    enum { arity = sizeof...(Args) };

    /**
    .. type:: type arg<n>::type

        The type of the *n*-th argument.
    */
    template <size_t i>
    struct arg
    {
        typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
    };
};

template <typename ReturnType, typename... Args>
struct function_traits<ReturnType(*)(Args...)>
    : public function_traits<ReturnType(Args...)>
{};

template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...)>
    : public function_traits<ReturnType(Args...)>
{
    typedef ClassType& owner_type;
};

template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...) const>
    : public function_traits<ReturnType(Args...)>
{
    typedef const ClassType& owner_type;
};

template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...) volatile>
    : public function_traits<ReturnType(Args...)>
{
    typedef volatile ClassType& owner_type;
};

template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...) const volatile>
    : public function_traits<ReturnType(Args...)>
{
    typedef const volatile ClassType& owner_type;
};

template <typename FunctionType>
struct function_traits<std::function<FunctionType>>
    : public function_traits<FunctionType>
{};

#if defined(_GLIBCXX_FUNCTIONAL)
#define MEM_FN_SYMBOL_XX0SL7G4Z0J std::_Mem_fn
#elif defined(_LIBCPP_FUNCTIONAL)
#define MEM_FN_SYMBOL_XX0SL7G4Z0J std::__mem_fn
#endif

#ifdef MEM_FN_SYMBOL_XX0SL7G4Z0J

template <typename R, typename C>
struct function_traits<MEM_FN_SYMBOL_XX0SL7G4Z0J<R C::*>>
    : public function_traits<R(C*)>
{};
template <typename R, typename C, typename... A>
struct function_traits<MEM_FN_SYMBOL_XX0SL7G4Z0J<R(C::*)(A...)>>
    : public function_traits<R(C*, A...)>
{};
template <typename R, typename C, typename... A>
struct function_traits<MEM_FN_SYMBOL_XX0SL7G4Z0J<R(C::*)(A...) const>>
    : public function_traits<R(const C*, A...)>
{};
template <typename R, typename C, typename... A>
struct function_traits<MEM_FN_SYMBOL_XX0SL7G4Z0J<R(C::*)(A...) volatile>>
    : public function_traits<R(volatile C*, A...)>
{};
template <typename R, typename C, typename... A>
struct function_traits<MEM_FN_SYMBOL_XX0SL7G4Z0J<R(C::*)(A...) const volatile>>
    : public function_traits<R(const volatile C*, A...)>
{};

#undef MEM_FN_SYMBOL_XX0SL7G4Z0J
#endif

template <typename T>
struct function_traits<T&> : public function_traits<T> {};
template <typename T>
struct function_traits<const T&> : public function_traits<T> {};
template <typename T>
struct function_traits<volatile T&> : public function_traits<T> {};
template <typename T>
struct function_traits<const volatile T&> : public function_traits<T> {};
template <typename T>
struct function_traits<T&&> : public function_traits<T> {};
template <typename T>
struct function_traits<const T&&> : public function_traits<T> {};
template <typename T>
struct function_traits<volatile T&&> : public function_traits<T> {};
template <typename T>
struct function_traits<const volatile T&&> : public function_traits<T> {};


#define FORWARD_RES_8QR485JMSBT \
    typename std::conditional< \
        std::is_lvalue_reference<R>::value, \
        T&, \
        typename std::remove_reference<T>::type&& \
    >::type

/**
.. function:: auto utils::forward_like<Like, T>(T&& t) noexcept

    Forward the reference *t* like the type of *Like*. That means, if *Like* is
    an lvalue (reference), this function will return an lvalue reference of *t*.
    Otherwise, if *Like* is an rvalue, this function will return an rvalue
    reference of *t*.

    This is mainly used to propagate the expression category (lvalue/rvalue) of
    a member of *Like*, generalizing ``std::forward``.
*/
template <typename R, typename T>
FORWARD_RES_8QR485JMSBT forward_like(T&& input) noexcept
{
    return static_cast<FORWARD_RES_8QR485JMSBT>(input);
}

#undef FORWARD_RES_8QR485JMSBT

/**
.. type:: struct utils::copy_cv<From, To>

    Copy the CV qualifier between the two types. For example,
    ``utils::copy_cv<const int, double>::type`` will become ``const double``.
*/
template <typename From, typename To>
struct copy_cv
{
private:
    typedef typename std::remove_cv<To>::type raw_To;
    typedef typename std::conditional<std::is_const<From>::value,
                                      const raw_To, raw_To>::type const_raw_To;
public:
    /**
    .. type:: type type

        Result of cv-copying.
    */
    typedef typename std::conditional<std::is_volatile<From>::value,
                                      volatile const_raw_To, const_raw_To>::type type;
};

/**
.. type:: struct utils::pointee<T>

    Returns the type by derefering an instance of *T*. This is a generalization
    of ``std::remove_pointer``, that it also works with iterators.
*/
template <typename T>
struct pointee
{
    /**
    .. type:: type type

        Result of dereferencing.
    */
    typedef typename std::remove_reference<decltype(*std::declval<T>())>::type type;
};

/**
.. function:: std::add_rvalue_reference<T>::type utils::rt_val<T>() noexcept

    Returns a value of type *T*. It is guaranteed to do nothing and will not
    throw a compile-time error, but using the returned result will cause
    undefined behavior.
*/
template <typename T>
typename std::add_rvalue_reference<T>::type rt_val() noexcept
{
    return std::move(*static_cast<T*>(nullptr));
}

}

}

#endif

