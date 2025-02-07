// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkURI_h
#define vtkURI_h

#include "vtkIOCoreModule.h" // For export macro
#include "vtkObject.h"
#include "vtkSmartPointer.h"  // For vtkSmartPointer
#include "vtkWrappingHints.h" // For VTK_WRAPEXCLUDE

#include <cstdlib> // for std::size_t
#include <memory>  // for std::unique_ptr
#include <string>  // for std::string

VTK_ABI_NAMESPACE_BEGIN

/**
 * @brief Represent an URI component
 *
 * An URI can have an empty but defined component.
 * This applies to authority, path, query and fragment.
 * This is mainly used for strong string reconstruction, example:
 * `vtkURI::Parse("file://?#")->ToString()` must return `"file://?#"`,
 * all components are empty, but defined, so they must be restored
 * when recomposition the URI string representation.
 * URI path is always defined for a valid URI.
 */
class VTKIOCORE_EXPORT vtkURIComponent
{
public:
  struct UndefinedTag
  {
  };
  static constexpr UndefinedTag Undefined{};

  /**
   * @brief Default constructor. Constructs an undefined component.
   */
  vtkURIComponent() = default;

  /**
   * @brief Default constructor. Constructs a defined component.
   * @param str The component value, may be empty.
   */
  vtkURIComponent(std::string str)
    : Value{ std::move(str) }
    , Defined{ true }
  {
  }

  /**
   * @brief Default constructor. Constructs a defined component.
   * @param str The component value, may be empty, but must not be nullptr.
   */
  vtkURIComponent(const char* str)
    : Value{ str }
    , Defined{ true }
  {
  }

  /**
   * @brief Constructs an undefined component. Use vtkURIComponent::Undefined
   */
  vtkURIComponent(UndefinedTag) {}

  ~vtkURIComponent() = default;
  vtkURIComponent(const vtkURIComponent&) = default;
  vtkURIComponent& operator=(const vtkURIComponent&) = default;
  vtkURIComponent(vtkURIComponent&&) = default;
  vtkURIComponent& operator=(vtkURIComponent&&) = default;

  /**
   * @return Return component value. Is empty if this is undefined.
   */
  const std::string& GetValue() const noexcept { return this->Value; }

  /**
   * @return `true` if this is defined, `false` otherwise
   */
  bool IsDefined() const noexcept { return this->Defined; }

  /**
   * @return `true` if this is defined, `false` otherwise
   */
  explicit operator bool() const noexcept { return this->Defined; }

  ///@{
  /**
   * @return Equality compararison of URI components.
   * Two components are equal if they are both defined and have the same value, or if they are both
   * undefined.
   */
  bool operator==(const vtkURIComponent& other) const noexcept
  {
    return this->Value == other.Value && this->Defined == other.Defined;
  }

  bool operator!=(const vtkURIComponent& other) const noexcept { return !(*this == other); }
  ///@}

private:
  std::string Value;
  bool Defined = false;
};

/**
 * @brief URI representation
 *
 * This class is final and immutable.
 * - Use `vtkURI::Parse` to create an URI from its string representation.
 * - Use `ToString` to get the string representation from an URI.
 * - Use `vtkURI::Make` to create an URI from components directly.
 * - Use `vtkURI::Resolve` to merge two URIs.
 * - Use `vtkURI::Clone` or member `Clone` if you need to copy an URI.
 *
 * Other functions are mainly getters for URI components or URI type identification.
 *
 * Known limitations:
 * - No [normalized comparison support](https://datatracker.ietf.org/doc/html/rfc3986#section-6.1)
 */
class VTKIOCORE_EXPORT vtkURI final : public vtkObject
{
public:
  vtkTypeMacro(vtkURI, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * @brief Construct a new vtkURI
   *
   * Default URI as a defined but empty path. Other components are undefined.
   *
   * @return New URI instance
   */
  static vtkURI* New();

  /**
   *  Calls `PercentEncode(str.data(), str.size())`
   *
   * @param str Input string to encode, may be empty.
   * @return Encodes string from `str`
   */
  static std::string PercentEncode(const std::string& str)
  {
    return PercentEncode(str.data(), str.size());
  }

  /**
   * @brief Encode a string into an URI compatible, percent-encoded, string
   *
   * Transform all bytes in `str` that are **not** part of
   * ["reserved"](https://datatracker.ietf.org/doc/html/rfc3986#section-2.2)
   * or ["unreserved"](https://datatracker.ietf.org/doc/html/rfc3986#section-2.3)
   * character sets into
   * [percent-encoded values](https://datatracker.ietf.org/doc/html/rfc3986#section-2.1).
   *
   * Note:
   * - All '%' in `str` will be replaced with "%25",
   *   even if they already represent a percent-encoded value
   *
   * @param str Input string to encode, may be null if size is 0.
   * @param size Input string size, may be 0.
   * @return Encoded string from `str`
   */
  static std::string PercentEncode(const char* str, std::size_t size);

  /**
   *  Calls `PercentDecode(str.data(), str.size())`
   *
   * @param str Input string to decode, may be empty.
   * @return Decoded string from `str`
   */
  static std::string PercentDecode(const std::string& str)
  {
    return PercentDecode(str.data(), str.size());
  }

  /**
   * @brief Decode percent-encoded values from given string
   *
   * [Percent-encoded values](https://datatracker.ietf.org/doc/html/rfc3986#section-2.1)
   * are used to store reserved characters in URIs.
   *
   * This function decode `str`, replacing `%HH` values with their real value.
   *
   * @param str Input string to decode, may be null if size is 0.
   * @param size Input string size, may be 0.
   * @return Decoded string from `str`
   */
  static std::string PercentDecode(const char* str, std::size_t size);

  /**
   * @brief Create a new vtkURI with specified components
   *
   * Syntax of components is checked in order to ensure that they respect
   * [RFC3986](https://datatracker.ietf.org/doc/html/rfc3986#section-3).
   *
   * If scheme is "data" (case-insensitive), the path is only checked
   * until the beginning of the data. This is done to prevent massive overhead when constructing
   * a big data URI. Data validation has to be performed by the decoding algorithm.
   * vtkURI::PercentDecode does the required checks for raw data URIs.
   *
   * Percent-encoded character are not decoded. Use `vtkURI::PercentEncode` if necessary.
   *
   * Tip: Parameters may be moved-in to prevent copy of big strings.
   * This function is not wrapped. If you need to construct an URI from a wrapper, use `Parse(str)`.
   *
   * @param scheme URI scheme, must not be empty if defined.
   * @param authority URI authority, may be defined, but empty.
   * @param path URI path, must be defined, but can be empty.
   * @param query URI query, may be defined, but empty.
   * @param fragment URI fragment, may be defined, but empty.
   * @return nullptr if URI syntax checks do not pass, otherwise a new vtkURI.
   */
  VTK_WRAPEXCLUDE static vtkSmartPointer<vtkURI> Make(
    vtkURIComponent scheme = vtkURIComponent::Undefined,
    vtkURIComponent authority = vtkURIComponent::Undefined, vtkURIComponent path = "",
    vtkURIComponent query = vtkURIComponent::Undefined,
    vtkURIComponent fragment = vtkURIComponent::Undefined);

  /**
   * @brief Clone a vtkURI
   *
   * @param other vtkURI to clone
   * @return if `other == nullptr` returns nullptr, otherwise returns a new vtkURI
   * with the exact same components as `other`
   */
  static vtkSmartPointer<vtkURI> Clone(const vtkURI* other);

  /**
   * @brief Create a new URI from a string.
   *
   * Perform as if by calling `vtkURI::Parse(uri.data(), uri.size())`.
   *
   * @param uri the URI string representation, may be empty.
   * @return nullptr if URI syntax checks do not pass, otherwise a new vtkURI.
   */
  static vtkSmartPointer<vtkURI> Parse(const std::string& uri)
  {
    return Parse(uri.data(), uri.size());
  }

  /**
   * @brief Create a new URI from a string.
   *
   * @param uri the URI string representation, must not be nullptr if `size > 0`
   * @param size the URI string representation size, may be `0`
   * @return nullptr if URI syntax checks do not pass, otherwise a new vtkURI.
   */
  static vtkSmartPointer<vtkURI> Parse(const char* uri, std::size_t size);

  /**
   * @brief Resolve an URI from a base URI
   *
   * This implements [RFC3986](https://datatracker.ietf.org/doc/html/rfc3986#section-5).
   * Base URI are used to compose absolute URIs from relative reference.
   *
   * @param baseURI the base URI, if nullptr, this function only checks if `uri` is a complete URI
   * @param uri relative reference that needs to be resolved from `baseURI`
   * @return nullptr if URI syntax checks do not pass, otherwise a new vtkURI.
   */
  static vtkSmartPointer<vtkURI> Resolve(const vtkURI* baseURI, const vtkURI* uri);

  /**
   * @brief URI scheme.
   */
  const vtkURIComponent& GetScheme() const { return this->Scheme; }

  /**
   * @brief URI authority.
   */
  const vtkURIComponent& GetAuthority() const { return this->Authority; }

  /**
   * @brief URI path.
   */
  const vtkURIComponent& GetPath() const { return this->Path; }

  /**
   * @brief URI query.
   */
  const vtkURIComponent& GetQuery() const { return this->Query; }

  /**
   * @brief URI fragment.
   */
  const vtkURIComponent& GetFragment() const { return this->Fragment; }

  ///@{
  /**
   * @brief URI types determination
   *
   * URI can be either:
   * - A full [URI](https://datatracker.ietf.org/doc/html/rfc3986#section-3):
   *   It has a scheme.
   * - an [URI reference](https://datatracker.ietf.org/doc/html/rfc3986#section-4.1):
   *   an URI that is either a relative reference or a full URI.
   * - a [relative reference](https://datatracker.ietf.org/doc/html/rfc3986#section-4.2),
   *   an URI that refers to data that has to be resolved from a base URI prior to loading.
   *   It does not define a scheme but defines at least one other component.
   * - an [absolute URI](https://datatracker.ietf.org/doc/html/rfc3986#section-4.3),
   *   an URI that can be used as a base URI.
   *   It defines a scheme and no fragment. It may define other components.
   * - a [same-document reference](https://datatracker.ietf.org/doc/html/rfc3986#section-4.4):
   *   an URI that defines only a fragment.
   * - An empty URI
   */
  bool IsReference() const { return this->IsRelative() || this->IsFull(); }

  bool IsRelative() const { return !this->Scheme; }

  bool IsAbsolute() const { return this->Scheme && !this->Fragment; }

  bool IsFull() const { return this->Scheme.IsDefined(); }

  bool IsSameDocRef() const
  {
    return !this->Scheme && !this->Authority && this->Path.GetValue().empty() && !this->Query &&
      this->Fragment;
  }

  bool IsEmpty() const
  {
    return !this->Scheme && !this->Authority && this->Path.GetValue().empty() && !this->Query &&
      !this->Fragment;
  }
  ///@}

  /**
   * @return `vtkURI::Clone(this)`
   */
  vtkSmartPointer<vtkURI> Clone() const { return vtkURI::Clone(this); }

  /**
   * @brief Construct the string representation of the URI
   *
   * @return a string representing the URI
   */
  std::string ToString() const;

private:
  /**
   * @brief Private version of vtkURI::Make but does not perform syntax check
   */
  static vtkSmartPointer<vtkURI> MakeUnchecked(vtkURIComponent scheme, vtkURIComponent authority,
    vtkURIComponent path, vtkURIComponent query, vtkURIComponent fragment);

  // These functions are factories and need write access to this class
  friend vtkSmartPointer<vtkURI> MakeUnchecked(vtkURIComponent scheme, vtkURIComponent authority,
    vtkURIComponent path, vtkURIComponent query, vtkURIComponent fragment);
  friend vtkSmartPointer<vtkURI> Clone(const vtkURI* other);

  vtkURI() = default;
  ~vtkURI() override = default;
  vtkURI(const vtkURI&) = delete;
  vtkURI& operator=(const vtkURI&) = delete;

  vtkURIComponent Scheme;
  vtkURIComponent Authority;
  vtkURIComponent Path{ "" }; // path is defined but empty by default
  vtkURIComponent Query;
  vtkURIComponent Fragment;
};

VTK_ABI_NAMESPACE_END

#endif
