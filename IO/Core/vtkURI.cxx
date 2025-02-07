// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkURI.h"

#include "vtkObjectFactory.h"
#include "vtkValueFromString.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <cstring>

VTK_ABI_NAMESPACE_BEGIN

namespace
{

// https://datatracker.ietf.org/doc/html/rfc3986#section-2.2
bool IsGenDelimiter(char c)
{
  return c == ':' || c == '/' || c == '?' || c == '#' || c == '[' || c == ']' || c == '@';
}

bool IsSubDelimiter(char c)
{
  return c == '!' || c == '$' || c == '&' || c == '\'' || c == '(' || c == ')' || c == '*' ||
    c == '+' || c == ',' || c == ';' || c == '=';
}

bool IsReservedCharacter(char c)
{
  return IsGenDelimiter(c) || IsSubDelimiter(c);
}

bool IsAlphaNumeric(char c)
{
  return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || ('0' <= c && c <= '9');
}

// https://datatracker.ietf.org/doc/html/rfc3986#section-2.3
bool IsUnreservedCharacter(char c)
{
  return IsAlphaNumeric(c) || c == '-' || c == '.' || c == '_' || c == '~';
}

// pchar = unreserved / pct-encoded / sub-delims / ":" / "@" / "/"
// pct-encoded is handled by PctFindIf
bool IsPChar(char c)
{
  return IsUnreservedCharacter(c) || IsSubDelimiter(c) || c == ':' || c == '@' || c == '/';
}

// The following functions extract one component
// and return an iterator one-past last component character
// They must be called in the right order to ensure coherency
// Scheme -> Authority -> Path -> Query -> Fragment
const char* ExtractScheme(const char* uri, const char* end, vtkURIComponent& output)
{
  const auto schemeEnd = std::find_if_not(uri, end,
    [](char c)
    { return std::isalnum(static_cast<unsigned char>(c)) || c == '+' || c == '-' || c == '.'; });

  if (schemeEnd == end || *schemeEnd != ':')
  {
    return uri; // not a scheme
  }

  output = std::string{ uri, schemeEnd };

  return schemeEnd + 1;
}

const char* ExtractAuthority(const char* uri, const char* end, vtkURIComponent& output)
{
  if (std::distance(uri, end) >= 2 && std::strncmp(uri, "//", 2) == 0)
  {
    uri += 2;

    const auto authEnd =
      std::find_if(uri, end, [](char c) { return c == '/' || c == '#' || c == '?'; });

    output = std::string{ uri, authEnd };

    return authEnd;
  }

  return uri;
}

const char* ExtractPath(const char* uri, const char* end, vtkURIComponent& output)
{
  const auto pathEnd = std::find_if(uri, end, [](char c) { return c == '#' || c == '?'; });

  output = std::string{ uri, pathEnd }; // always defined, but may be empty

  return pathEnd;
}

const char* ExtractQuery(const char* uri, const char* end, vtkURIComponent& output)
{
  const auto queryEnd = std::find_if(uri, end, [](char c) { return c == '#'; });

  if (uri == queryEnd) // empty query
  {
    return uri;
  }

  if (*uri != '?') // not a query
  {
    return uri;
  }

  output = std::string{ uri + 1, queryEnd };

  if (queryEnd == end)
  {
    return end;
  }

  return queryEnd;
}

const char* ExtractFragment(const char* uri, const char* end, vtkURIComponent& output)
{
  if (uri == end) // empty fragment
  {
    return uri;
  }

  if (*uri != '#') // not a fragment ? Error ?
  {
    return uri;
  }

  output = std::string{ uri + 1, end };

  return end;
}

bool IsHexDigit(char c)
{
  return ('A' <= c && c <= 'F') || ('a' <= c && c <= 'f') || ('0' <= c && c <= '9');
}

// https://datatracker.ietf.org/doc/html/rfc3986#section-2.1
template <typename It>
bool IsPercentEncodedValue(It begin, It end)
{
  if (std::distance(begin, end) < 3)
  {
    return false;
  }

  if (*begin != '%')
  {
    return false;
  }

  ++begin;
  if (!IsHexDigit(*begin))
  {
    return false;
  }

  ++begin;
  if (!IsHexDigit(*begin))
  {
    return false;
  }

  return true;
}

// std::find_if that interprets and skips percent-encoded values ('%XX')
template <typename It, typename Pred>
It PctFindIf(It begin, It end, Pred&& pred)
{
  while (begin != end)
  {
    if (*begin == '%')
    {
      if (!IsPercentEncodedValue(begin, end))
      {
        return begin;
      }

      std::advance(begin, 3);
    }
    else if (pred(*begin))
    {
      return begin;
    }
    else
    {
      ++begin;
    }
  }

  return end;
}

// https://datatracker.ietf.org/doc/html/rfc3986#section-3.1
bool CheckSchemeSyntax(const vtkURIComponent& comp)
{
  if (!comp) // may be undefined
  {
    return true;
  }

  const auto& scheme = comp.GetValue();
  if (scheme.empty()) // not valid for scheme !!
  {
    vtkErrorWithObjectMacro(nullptr, "URI scheme must not be empty if defined");
    return false;
  }

  if (!std::isalpha(static_cast<unsigned char>(scheme.front())))
  {
    vtkErrorWithObjectMacro(nullptr, "URI scheme must start with a letter");
    return false;
  }

  auto illegalChar = std::find_if_not(scheme.begin(), scheme.end(),
    [](char c)
    { return std::isalnum(static_cast<unsigned char>(c)) || c == '+' || c == '-' || c == '.'; });

  if (illegalChar != scheme.end())
  {
    vtkErrorWithObjectMacro(nullptr, "Reserved char '" << *illegalChar << "' found in URI scheme");
    return false;
  }

  return true;
}

struct AuthorityInfo
{
  std::string UserInfo;
  std::string Host;
  std::string Port;
};

AuthorityInfo ExtractAuthorityInfo(const char* auth, const char* end)
{
  AuthorityInfo output;

  const auto userInfoEnd = std::find(auth, end, '@');
  if (userInfoEnd != end) // user info provided
  {
    output.UserInfo = std::string{ auth, userInfoEnd };
    auth = userInfoEnd;
  }

  const auto hostEnd = std::find(auth, end, ':');
  output.Host = std::string{ auth, hostEnd };

  if (hostEnd != end)
  {
    output.Port = std::string{ hostEnd + 1, end };
  }

  return output;
}

// https://datatracker.ietf.org/doc/html/rfc3986#section-3.2
bool CheckAuthoritySyntax(const vtkURIComponent& comp)
{
  if (!comp || comp.GetValue().empty()) // empty is valid
  {
    return true;
  }

  const auto& auth = comp.GetValue();

  AuthorityInfo info = ExtractAuthorityInfo(auth.data(), auth.data() + auth.size());

  // userinfo = *( unreserved / pct-encoded / sub-delims / ":" )
  if (!info.UserInfo.empty())
  {
    const auto it = PctFindIf(info.UserInfo.begin(), info.UserInfo.end(),
      [](char c) { return !(IsUnreservedCharacter(c) || IsSubDelimiter(c) || c == ':'); });

    if (it != info.UserInfo.end())
    {
      vtkErrorWithObjectMacro(
        nullptr, "Authority user info contains reserved character '" << *it << "'");
      return false;
    }
  }

  if (!info.Host.empty())
  {
    // IP-literal = "[" *(unreserved / sub-delims / ":") "]"
    if (info.Host.front() == '[' && info.Host.back() == ']')
    {
      const auto it = std::find_if(info.Host.begin() + 1, info.Host.end() - 1,
        [](char c) { return IsUnreservedCharacter(c) || IsSubDelimiter(c) || c == ':'; });

      if (it != info.Host.end() - 1)
      {
        vtkErrorWithObjectMacro(nullptr, "Invalid character in host IP '" << *it << "'");
        return false;
      }
    }
    else
    {
      // reg-name = *( unreserved / pct-encoded / sub-delims )
      const auto it = PctFindIf(info.Host.begin(), info.Host.end(),
        [](char c) { return !(IsUnreservedCharacter(c) || IsSubDelimiter(c)); });

      if (it != info.Host.end())
      {
        vtkErrorWithObjectMacro(nullptr, "Invalid character in host '" << *it << "'");
        return false;
      }
    }
  }

  if (!info.Port.empty())
  {
    // port = *DIGIT
    auto it = std::find_if(info.Port.begin(), info.Port.end(),
      [](char c) { return std::isdigit(static_cast<unsigned char>(c)); });

    if (it != info.Port.end())
    {
      vtkErrorWithObjectMacro(
        nullptr, "Non digit character find in authority port '" << *it << "'");
      return false;
    }
  }

  return true;
}

// https://datatracker.ietf.org/doc/html/rfc3986#section-3.3
bool CheckPathSyntax(const vtkURIComponent& comp, bool hasAuthority, bool isDataURI)
{
  if (!comp)
  {
    vtkErrorWithObjectMacro(nullptr, "URI path can not be undefined");
    return false;
  }

  if (comp.GetValue().empty()) // empty is valid
  {
    return true;
  }

  if (hasAuthority && comp.GetValue().front() != '/')
  {
    vtkErrorWithObjectMacro(
      nullptr, "If an authority is defined, path must be empty or start with '/'");
    return false;
  }

  const auto& path = comp.GetValue();

  if (isDataURI)
  {
    // Skip checks for data itself
    const auto typeEnd = std::find_if(path.begin(), path.end(), [](char c) { return c == ','; });

    auto it = PctFindIf(path.begin(), typeEnd, [](char c) { return !IsPChar(c); });

    if (it != typeEnd)
    {
      vtkErrorWithObjectMacro(nullptr, "Invalid character in path component '" << *it << "'");
      return false;
    }
  }
  else
  {
    auto it = PctFindIf(path.begin(), path.end(), [](char c) { return !IsPChar(c); });

    if (it != path.end())
    {
      vtkErrorWithObjectMacro(nullptr, "Invalid character in path component '" << *it << "'");
      return false;
    }
  }

  return true;
}

// https://datatracker.ietf.org/doc/html/rfc3986#section-3.4
// Fragment use same syntax as query: https://datatracker.ietf.org/doc/html/rfc3986#section-3.5
bool CheckQueryOrFragmentSyntax(const vtkURIComponent& comp)
{
  if (!comp || comp.GetValue().empty()) // empty is valid
  {
    return true;
  }

  const auto& str = comp.GetValue();
  // query = fragment = *( pchar / "?" )
  auto it = PctFindIf(str.begin(), str.end(), [](char c) { return !(IsPChar(c) || c == '?'); });

  if (it != str.end())
  {
    vtkErrorWithObjectMacro(
      nullptr, "Invalid character in query or fragment component '" << *it << "'");
    return false;
  }

  return true;
}

// Generic check that can be applied to any URI:
bool CheckURISyntax(const vtkURI& uri)
{
  if (!CheckSchemeSyntax(uri.GetScheme()))
  {
    return false;
  }

  if (!CheckAuthoritySyntax(uri.GetAuthority()))
  {
    return false;
  }

  const bool isDataURI = vtksys::SystemTools::LowerCase(uri.GetScheme().GetValue()) == "data";
  if (!CheckPathSyntax(uri.GetPath(), uri.GetAuthority().IsDefined(), isDataURI))
  {
    return false;
  }

  if (!CheckQueryOrFragmentSyntax(uri.GetQuery()))
  {
    return false;
  }

  if (!CheckQueryOrFragmentSyntax(uri.GetFragment()))
  {
    return false;
  }

  return true;
}

std::string RemoveDotSegments(std::string input)
{
  // https://datatracker.ietf.org/doc/html/rfc3986#section-5.2.4
  std::string output;
  output.reserve(input.size());

  while (!input.empty())
  {
    if (input.find("../") == 0)
    {
      input.erase(0, 3);
    }
    else if (input.find("./") == 0)
    {
      input.erase(0, 2);
    }
    else if (input.find("/./") == 0)
    {
      input.erase(0, 2);
    }
    else if (input == "/.")
    {
      input.clear();
      if (output.back() != '/')
      {
        output += '/';
      }
    }
    else if (input.find("/../") == 0)
    {
      input.erase(0, 3);
      auto lastSegment = output.rfind('/');
      if (lastSegment != std::string::npos)
      {
        output.erase(lastSegment);
      }
    }
    else if (input == "/..")
    {
      input.clear();
      auto lastSegment = output.rfind('/');
      if (lastSegment != std::string::npos)
      {
        output.erase(lastSegment + 1);
      }
    }
    else if (input == "." || input == "..")
    {
      input.clear();
    }
    else
    {
      std::size_t firstSegment = input.find('/', 1);
      output += input.substr(0, firstSegment);
      input.erase(0, firstSegment);
    }
  }

  return output;
}

std::string MergeWithBasePath(const vtkURI& base, const std::string& path)
{
  // https://datatracker.ietf.org/doc/html/rfc3986#section-5.2.3
  const auto& basePath = base.GetPath().GetValue();
  if (!base.GetAuthority().IsDefined() && basePath.empty())
  {
    return "/" + path;
  }

  const auto lastSlash = basePath.rfind('/');
  if (lastSlash != std::string::npos)
  {
    return basePath.substr(0, lastSlash + 1) + path;
  }

  return path;
}

} // namespace

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkURI);

//------------------------------------------------------------------------------
std::string vtkURI::PercentEncode(const char* str, std::size_t size)
{
  if (size != 0 && !str)
  {
    vtkErrorWithObjectMacro(nullptr, "Null string with non-null size");
    return {};
  }

  // LUT for value -> hex conversion
  static constexpr std::array<char, 16> HexCharsLUT = { '0', '1', '2', '3', '4', '5', '6', '7', '8',
    '9', 'A', 'B', 'C', 'D', 'E', 'F' };

  std::string output;
  output.reserve(size);

  for (std::size_t i{}; i < size; ++i)
  {
    const char current = str[i];

    if (IsReservedCharacter(current) || IsUnreservedCharacter(current))
    {
      output += current;
    }
    else
    {
      const auto value = static_cast<unsigned int>(current);

      output += '%';
      output += HexCharsLUT[((value >> 4u) & 0x0Fu)];
      output += HexCharsLUT[(value & 0x0Fu)];
    }
  }

  return output;
}

//------------------------------------------------------------------------------
std::string vtkURI::PercentDecode(const char* str, std::size_t size)
{
  if (size != 0 && !str)
  {
    vtkErrorWithObjectMacro(nullptr, "Null string with non-null size");
    return {};
  }

  std::string output;
  output.reserve(size);

  std::array<char, 4> temp = { '0', 'x', 0, 0 }; // used for hex decoding

  for (std::size_t i = 0; i < size; ++i)
  {
    const char current = str[i];

    if (current == '%') // decode percent encoded byte
    {
      if (size - i < 3)
      {
        vtkErrorWithObjectMacro(nullptr, "Truncated percent-encoded value");
        return {};
      }

      temp[2] = str[i + 1];
      temp[3] = str[i + 2];
      unsigned char value;
      if (vtkValueFromString(temp.data(), temp.data() + temp.size(), value) != temp.size())
      {
        vtkErrorWithObjectMacro(nullptr, "Invalid value %" << temp[2] << temp[3] << " in URI data");
        return {};
      }

      output += static_cast<char>(value);
      i += 2;
    }
    else if (!IsReservedCharacter(current) && !IsUnreservedCharacter(current))
    {
      vtkErrorWithObjectMacro(nullptr, "Invalid character '" << current << "' in URI data");
      return {};
    }
    else
    {
      output += current;
    }
  }

  return output;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkURI> vtkURI::Make(vtkURIComponent scheme, vtkURIComponent authority,
  vtkURIComponent path, vtkURIComponent query, vtkURIComponent fragment)
{
  auto output = MakeUnchecked(std::move(scheme), std::move(authority), std::move(path),
    std::move(query), std::move(fragment));

  if (!CheckURISyntax(*output))
  {
    return nullptr;
  }

  return output;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkURI> vtkURI::Clone(const vtkURI* other)
{
  if (!other)
  {
    return nullptr;
  }

  auto output = vtkSmartPointer<vtkURI>::New();
  output->Scheme = other->Scheme;
  output->Authority = other->Authority;
  output->Path = other->Path;
  output->Query = other->Query;
  output->Fragment = other->Fragment;

  return output;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkURI> vtkURI::Parse(const char* uri, std::size_t size)
{
  if (size != 0 && !uri)
  {
    vtkErrorWithObjectMacro(nullptr, "Null string with non-null size");
    return nullptr;
  }

  const auto end = uri + size;

  vtkURIComponent scheme;
  uri = ExtractScheme(uri, end, scheme);
  vtkURIComponent auth;
  uri = ExtractAuthority(uri, end, auth);
  vtkURIComponent path;
  uri = ExtractPath(uri, end, path);
  vtkURIComponent query;
  uri = ExtractQuery(uri, end, query);
  vtkURIComponent frag;
  uri = ExtractFragment(uri, end, frag);

  return vtkURI::Make(
    std::move(scheme), std::move(auth), std::move(path), std::move(query), std::move(frag));
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkURI> vtkURI::Resolve(const vtkURI* baseURI, const vtkURI* uri)
{
  if (!uri)
  {
    vtkErrorWithObjectMacro(nullptr, "uri must not be null");
    return nullptr;
  }

  if (!baseURI) // resolve(none, x) == x
  {
    if (!uri->IsFull()) // must be full after resolution
    {
      return nullptr;
    }

    return vtkURI::Clone(uri);
  }

  // https://datatracker.ietf.org/doc/html/rfc3986#section-5.2.2
  vtkURIComponent scheme;
  vtkURIComponent auth;
  vtkURIComponent path;
  vtkURIComponent query;

  if (uri->GetScheme())
  {
    scheme = uri->GetScheme();
    auth = uri->GetAuthority();

    // Data URIs can contain slashes (both base64 and raw).
    // This highly impact performances of RemoveDotSegments.
    // This bypass should never impact observable behavior since a valid data URI
    // will never contain dot or dot-dot path segments.
    if (vtksys::SystemTools::LowerCase(uri->GetScheme().GetValue()) == "data")
    {
      path = uri->GetPath().GetValue();
    }
    else
    {
      path = RemoveDotSegments(uri->GetPath().GetValue());
    }

    query = uri->GetQuery();
  }
  else
  {
    scheme = baseURI->GetScheme();

    if (uri->GetAuthority())
    {
      auth = uri->GetAuthority();
      path = RemoveDotSegments(uri->GetPath().GetValue());
      query = uri->GetQuery();
    }
    else
    {
      auth = baseURI->GetAuthority();

      if (uri->GetPath().GetValue().empty())
      {
        path = baseURI->GetPath();
        if (uri->GetQuery())
        {
          query = uri->GetQuery();
        }
        else
        {
          query = baseURI->GetQuery();
        }
      }
      else
      {
        if (uri->GetPath().GetValue().front() == '/')
        {
          path = RemoveDotSegments(uri->GetPath().GetValue());
        }
        else
        {
          path = RemoveDotSegments(MergeWithBasePath(*baseURI, uri->GetPath().GetValue()));
        }

        query = uri->GetQuery();
      }
    }
  }

  vtkURIComponent frag = uri->GetFragment();

  // Will always be valid since both inputs will already be checked
  auto output = vtkURI::MakeUnchecked(
    std::move(scheme), std::move(auth), std::move(path), std::move(query), std::move(frag));

  if (!output->IsFull())
  {
    vtkErrorWithObjectMacro(nullptr,
      "Failed to resolve URI \"" << uri->ToString() << "\" from base URI \"" << baseURI->ToString()
                                 << "\". Result \"" << output->ToString() << "\" is incomplete");
    return nullptr;
  }

  return output;
}

//------------------------------------------------------------------------------
std::string vtkURI::ToString() const
{
  // https://datatracker.ietf.org/doc/html/rfc3986#section-5.3
  std::string output;

  if (this->Scheme)
  {
    output += this->Scheme.GetValue();
    output += ':';
  }

  if (this->Authority)
  {
    output += "//";
    output += this->Authority.GetValue();
  }

  output += this->Path.GetValue();

  if (this->Query)
  {
    output += '?';
    output += this->Query.GetValue();
  }

  if (this->Fragment)
  {
    output += '#';
    output += this->Fragment.GetValue();
  }

  return output;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkURI> vtkURI::MakeUnchecked(vtkURIComponent scheme, vtkURIComponent authority,
  vtkURIComponent path, vtkURIComponent query, vtkURIComponent fragment)
{
  auto output = vtkSmartPointer<vtkURI>::New();
  output->Scheme = std::move(scheme);
  output->Authority = std::move(authority);
  output->Path = std::move(path);
  output->Query = std::move(query);
  output->Fragment = std::move(fragment);

  return output;
}

//------------------------------------------------------------------------------
void vtkURI::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Uri: " << this->ToString() << std::endl;
  Superclass::PrintSelf(os, indent.GetNextIndent());
}
VTK_ABI_NAMESPACE_END
