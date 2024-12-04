// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStdString
 * @brief   Wrapper around std::string to keep symbols short.
 *
 * vtkStdString derives from std::string to provide shorter symbol
 * names than basic_string<...> in namespace std given by the standard
 * STL string.
 */

#ifndef vtkStdString_h
#define vtkStdString_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkSystemIncludes.h"   // For VTKCOMMONCORE_EXPORT.
#include <string>                // For the superclass.
#include <utility>               // For std::move

VTK_ABI_NAMESPACE_BEGIN
class vtkStdString;
VTKCOMMONCORE_EXPORT ostream& operator<<(ostream&, const vtkStdString&);

class vtkStdString : public std::string
{
public:
  typedef std::string StdString;
  typedef StdString::value_type value_type;
  typedef StdString::pointer pointer;
  typedef StdString::reference reference;
  typedef StdString::const_reference const_reference;
  typedef StdString::size_type size_type;
  typedef StdString::difference_type difference_type;
  typedef StdString::iterator iterator;
  typedef StdString::const_iterator const_iterator;
  typedef StdString::reverse_iterator reverse_iterator;
  typedef StdString::const_reverse_iterator const_reverse_iterator;

  vtkStdString() = default;
  vtkStdString(const value_type* s)
    : std::string(s)
  {
  }
  vtkStdString(const value_type* s, size_type n)
    : std::string(s, n)
  {
  }
  vtkStdString(const std::string& s)
    : std::string(s)
  {
  }
  vtkStdString(std::string&& s)
    : std::string(std::move(s))
  {
  }
  vtkStdString(const std::string& s, size_type pos, size_type n = std::string::npos)
    : std::string(s, pos, n)
  {
  }
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkStdString.h
