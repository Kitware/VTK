//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================

#ifndef token_TypeContainer_h
#define token_TypeContainer_h

#include "token/Token.h" // For Token used as container keys.
#include "token/Type.h" // For TypeName/TypeToken as well as Compiler.h.

#include <memory>
#include <stdexcept>
#include <unordered_set>
#include <string>
#include <typeinfo>
#include <unordered_map>

token_BEGIN_NAMESPACE

/**\brief A container for caching and retrieving instances of types.
  *
  * Instances are retrieved using type information as a key, allowing for
  * simultaneous lookup and static downcast into that type. TypeContainer
  * supports copying by cloning its elements using thier copy constructors.
  */
class TOKEN_EXPORT TypeContainer
{
  struct Wrapper
  {
    virtual ~Wrapper() = default;
    virtual std::unique_ptr<Wrapper> clone() const = 0;
  };

  template<typename Type>
  struct WrapperFor : Wrapper
  {
    template<typename... Args>
    WrapperFor(Args&&... v)
      : value(std::forward<Args>(v)...)
    {
    }

    std::unique_ptr<Wrapper> clone() const override
    {
#ifdef token_HAVE_CXX_14
      return std::make_unique<WrapperFor<Type>>(std::make_unique<Type>(*value));
#else
      return std::unique_ptr<Wrapper>(new WrapperFor<Type>(new Type(*value)));
#endif
    }

    std::unique_ptr<Type> value;
  };

public:
  class BadTypeError : public std::out_of_range
  {
  public:
    BadTypeError(const std::string& typeName)
      : std::out_of_range("Type \"" + typeName + "\" not available in this container")
    {
    }
  };

  using KeyType = Hash;

  /// Construct an empty TypeContainer.
  TypeContainer() = default;

  /// Construct a TypeContainer whose contents are copied from an existing
  /// TypeContainer.
  TypeContainer(const TypeContainer&);

  /// Move the contents of one TypeContainer into a new TypeContainer.
  TypeContainer(TypeContainer&&) = default;

  /// Copy the contents of an existing TypeContainer into this one.
  TypeContainer& operator=(const TypeContainer&);

  /// Move the contents of an existing TypeContainer into this one.
  TypeContainer& operator=(TypeContainer&&) = default;

  /// Construct a TypeContainer instance from any number of elements. Elements
  /// are added in the order they appear in the constructor, so subsequent values
  /// for the same type will be ignored.
  template<
    typename Arg,
    typename... Args,
    typename std::enable_if<!std::is_base_of<TypeContainer, Arg>::value, int>::type = 0>
  TypeContainer(const Arg& arg, const Args&... args)
  {
    insertAll(arg, args...);
  }

  virtual ~TypeContainer();

  /// Return the ID used to index a given \a Type.
  template<typename Type>
  KeyType keyId() const
  {
    (void)this; // Prevent clang-tidy from complaining this could be class-static.
    std::string keyName = typeName<Type>();
    KeyType value = Token(keyName).getId();
    return value;
  }

  /// Obtain a key hash ID for \a Type without managing its string.
  ///
  /// This is safe to call even after the token::Manager has been
  /// destroyed during finalization since it does not attempt to
  /// insert the string into the token::Manager. It should be used
  /// when removing or testing the existence of entries, but not for
  /// inserting entries since then the key-names will be unavailable.
  template<typename Type>
  KeyType safeKeyId() const
  {
    (void)this; // Prevent clang-tidy from complaining this could be class-static.
    std::string keyName = typeName<Type>();
    KeyType value = Token::stringHash(keyName.c_str(), keyName.size());
    return value;
  }

  /// Check if a Type is present in the TypeContainer.
  template<typename Type>
  bool contains() const
  {
    return (m_container.find(this->safeKeyId<Type>()) != m_container.end());
  }

  /// Insert a Type instance into the TypeContainer.
  /// Note that if the type already exists in the container, the insertion will fail.
  template<typename Type>
  bool insert(const Type& value)
  {
    return m_container
      .emplace(std::make_pair(
        this->keyId<Type>(),
#ifdef token_HAVE_CXX_14
        std::make_unique<WrapperFor<Type>>(std::make_unique<Type>(value))
#else
        std::unique_ptr<Wrapper>(new WrapperFor<Type>(std::unique_ptr<Type>(new Type((value)))))
#endif
          ))
      .second;
  }

  /// Insert a Type instance into the TypeContainer if it does not exist already or replace it if it does.
  template<typename Type>
  bool insert_or_assign(const Type& value)
  {
    if (this->contains<Type>())
    {
      this->erase<Type>();
    }
    return this->insert<Type>(value);
  }

  /// Emplace a Type instance into the TypeContainer.
  template<typename Type, typename... Args>
  bool emplace(Args&&... args)
  {
    return m_container
      .emplace(std::make_pair(
        this->keyId<Type>(),
#ifdef token_HAVE_CXX_14
        std::make_unique<WrapperFor<Type>>(std::make_unique<Type>(std::forward<Args>(args)...))
#else
        std::unique_ptr<Wrapper>(
          new WrapperFor<Type>(std::unique_ptr<Type>(new Type(std::forward<Args>(args)...))))
#endif
          ))
      .second;
  }

  /// Access a Type instance, and throw if it is not in the TypeContainer.
  template<typename Type>
  const Type& get() const
  {
    auto search = m_container.find(this->keyId<Type>());
    if (search == m_container.end())
    {
      throw BadTypeError(typeName<Type>());
    }

    return *(static_cast<WrapperFor<Type>*>(search->second.get()))->value;
  }

  /// For default-constructible types, access a Type instance, creating one if it
  /// is not in the TypeContainer.
  template<typename Type>
  typename std::enable_if<std::is_default_constructible<Type>::value, Type&>::type get() noexcept
  {
    auto search = m_container.find(this->keyId<Type>());
    if (search == m_container.end())
    {
      search = m_container
                 .emplace(std::make_pair(
                   this->keyId<Type>(),
#ifdef token_HAVE_CXX_14
                   std::make_unique<WrapperFor<Type>>(std::make_unique<Type>())
#else
                   std::unique_ptr<Wrapper>(new WrapperFor<Type>(std::unique_ptr<Type>(new Type)))
#endif
                     ))
                 .first;
    }

    return *(static_cast<WrapperFor<Type>*>(search->second.get()))->value;
  }

  /// For non-default-constructible types, access a Type instance; throw if it is
  /// not in the TypeContainer.
  template<typename Type>
  typename std::enable_if<!std::is_default_constructible<Type>::value, Type&>::type get()
  {
    auto search = m_container.find(this->keyId<Type>());
    if (search == m_container.end())
    {
      throw BadTypeError(typeName<Type>());
    }

    return *(static_cast<WrapperFor<Type>*>(search->second.get()))->value;
  }

  /// Remove a specific type of object from the container.
  template<typename Type>
  bool erase()
  {
    return m_container.erase(this->safeKeyId<Type>()) > 0;
  }

  /// Return true if the container holds no objects and false otherwise.
  bool empty() const noexcept { return m_container.empty(); }

  /// Return the number of objects held by the container.
  std::size_t size() const noexcept { return m_container.size(); }

  /// Erase all objects held by the container.
  void clear() noexcept { m_container.clear(); }

  /// Return a set of keys corresponding to the values in the container.
  ///
  /// There is no run-time method to extract the values given just a key
  /// since the type is unknown; however, this does make it possible for
  /// the python layer to invoke a function adapter for specific types of
  /// objects held in the container to be fetched.
  std::unordered_set<Token> keys() const
  {
    std::unordered_set<Token> result;
    for (const auto& entry : m_container)
    {
      try
      {
        result.insert(Token(entry.first));
      }
      catch (std::invalid_argument&)
      {
        // Ignore entries with no matching string.
      }
    }
    return result;
  }

private:
  template<typename Arg, typename... Args>
  typename std::enable_if<!std::is_base_of<TypeContainer, Arg>::value, bool>::type insertAll(
    const Arg& arg,
    const Args&... args)
  {
    return insert<Arg>(arg) && insertAll(args...);
  }
  bool insertAll() { return true; }

  std::unordered_map<std::size_t, std::unique_ptr<Wrapper>> m_container;
};
token_CLOSE_NAMESPACE

#endif
