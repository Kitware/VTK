/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnicodeString.h

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkUnicodeString - String class that stores Unicode text.
//
// .SECTION Description
// vtkUnicodeString provides storage for Unicode text.  Conceptually, it
// acts as a container for a sequence of Unicode characters, providing a
// public interface similar to basic_string<>.  For character-oriented
// operations, this means reading / writing 32-bit UTF-32 / UCS-4 characters.
// Internally, characters may be stored using variable-length encodings for
// efficiency. Explicit conversions to-and-from other encodings are provided,
// and implicit conversions are deliberately avoided, to avoid confusion.
//
// Note that, because vtkUnicodeString uses variable-length encodings for
// storage, character-oriented operations will generally provide O(N) access
// instead of O(1).
//
// The current implementation stores the sequence with UTF-8 encoding, but
// this choice is subject to change and might become a compile-time or
// run-time option.
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef vtkUnicodeString_h
#define vtkUnicodeString_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkSystemIncludes.h"
#include <string>
#include <vector>

class vtkUnicodeString;

typedef vtkTypeUInt32 vtkUnicodeStringValueType;

//
// The following should be eventually placed in vtkSetGet.h
//

// This is same as extra extended template macro with an
// additional case for VTK_UNICODE_STRING
#define vtkSuperExtraExtendedTemplateMacro(call)                                 \
  vtkExtraExtendedTemplateMacro(call);                                            \
  vtkTemplateMacroCase(VTK_UNICODE_STRING, vtkUnicodeString, call)

class VTKCOMMONCORE_EXPORT vtkUnicodeString
{
public:
  typedef vtkUnicodeStringValueType value_type;
  typedef std::string::size_type size_type;

  class VTKCOMMONCORE_EXPORT const_iterator
  {
  public:
    typedef std::bidirectional_iterator_tag iterator_category;
    typedef vtkUnicodeStringValueType value_type;
    typedef std::string::difference_type difference_type;
    typedef value_type* pointer;
    typedef value_type& reference;

    const_iterator();

    value_type operator*() const;
    bool operator==(const const_iterator&) const;
    bool operator!=(const const_iterator&) const;
    const_iterator& operator++();
    const_iterator operator++(int);
    const_iterator& operator--();
    const_iterator operator--(int);

  private:
    const_iterator(std::string::const_iterator);
    friend class vtkUnicodeString;
    std::string::const_iterator Position;
  };

  // Description:
  // Constructs an empty string.
  vtkUnicodeString();
  // Description:
  // Makes a deep-copy of another sequence.
  vtkUnicodeString(const vtkUnicodeString&);
  // Description:
  // Constructs a sequence of repeated characters.  Note: throws an exception if
  // the character isn't a valid Unicode code point.
  vtkUnicodeString(size_type count, value_type character);
  // Description:
  // Constructs a string from a sequence of Unicode characters.
  vtkUnicodeString(const_iterator begin, const_iterator end);

  // Description:
  // Tests a sequence of bytes, returning true iff they are a valid UTF-8 sequence.
  static bool is_utf8(const char*);
  static bool is_utf8(const std::string&);

  // Description:
  // Constructs a string from a null-terminated sequence of UTF-8 encoded characters.
  static vtkUnicodeString from_utf8(const char*);
  // Constructs a string from a half-open sequence of UTF-8 encoded characters.
  static vtkUnicodeString from_utf8(const char* begin, const char* end);
  // Constructs a string from a sequence of UTF-8 encoded characters.
  static vtkUnicodeString from_utf8(const std::string&);
  // Description:
  // Constructs a string from a null-terminated sequence of UTF-16 encoded characters.
  static vtkUnicodeString from_utf16(const vtkTypeUInt16*);

  // Description:
  // Replaces the current sequence with a deep copy of another.
  vtkUnicodeString& operator=(const vtkUnicodeString&);

  // Description:
  // Returns a forward iterator that points at the first element of the sequence
  // (or just beyond the end of an empty sequence).
  const_iterator begin() const;
  // Description:
  // Returns a forward iterator that points just beyond the end of the sequence.
  const_iterator end() const;

  // Description:
  // Returns the Unicode character at the given character offset within the sequence,
  // or throws std::out_of_range if the offset is invalid.
  value_type at(size_type offset) const;
  // Description:
  // Returns the Unicode character at the given character offset within the sequence.
  // Behavior is undefined if the position is invalid.
  value_type operator[](size_type offset) const;

  // Description:
  // Returns the sequence as a null-terminated sequence of UTF-8 encoded characters.
  const char* utf8_str() const;
  // Description:
  // Inserts the sequence into the supplied storage as a collection of UTF-8 encoded
  // characters.
  void utf8_str(std::string& result) const;
  // Description:
  // Returns the sequence as a collection of UTF-16 encoded characters.
  std::vector<vtkTypeUInt16> utf16_str() const;
  // Description:
  // Inserts the sequence into the supplied storage as a collection of UTF-16 encoded
  // characters
  void utf16_str(std::vector<vtkTypeUInt16>& result) const;

  // Description:
  // Returns the number of bytes (not characters) in the sequence.
  size_type byte_count() const;
  // Description:
  // Returns the number of characters (not bytes) in the sequence.
  size_type character_count() const;
  // Description:
  // Returns true if the string contains an empty sequence.
  bool empty() const;

  // Description:
  // The largest representable value of size_type, used as a special-code.
  static const size_type npos;

  // Description:
  // Append a Unicode character to the end of the sequence.
  vtkUnicodeString& operator+=(value_type);
  // Description:
  // Append a Unicode sequence to the end of the current sequence.
  vtkUnicodeString& operator+=(const vtkUnicodeString& rhs);

  // Description:
  // Append a Unicode character to the end of the sequence.
  void push_back(value_type);

  // Description:
  // Append Unicode to the current sequence.
  void append(const vtkUnicodeString& value);
  void append(size_type count, value_type character);
  void append(const_iterator begin, const_iterator end);

  // Description:
  // Replace the current sequence with another.
  void assign(const vtkUnicodeString& value);
  void assign(size_type count, value_type character);
  void assign(const_iterator begin, const_iterator end);

  // Description:
  // Resets the string to an empty sequence
  void clear();

  // Description:
  // Returns a copy of the current sequence, modified so that differences in case are
  // eliminated.  Thus, you can run fold_case() on two strings and then comparse them
  // to obtain a case-insensitive comparison.  Note that the string returned by
  // fold_case() may be larger than the original source sequence,
  //
  // See http://www.unicode.org/Public/UNIDATA/CaseFolding.txt for details.
  vtkUnicodeString fold_case() const;

  // Description:
  // Returns a negative value if the sequence compares less-than the
  // operand sequence, zero if the two sequences compare equal, or
  // a positive value otherwise.  Note that the definition of "less-than"
  // is undefined, so you should use some other method if you wish to
  // establish a specific ordering (such as alphabetical).
  int compare(const vtkUnicodeString&) const;

  // Description:
  // Returns a subset of the current sequence that up-to 'count' characters in length,
  // starting at character 'offset'.
  vtkUnicodeString substr(size_type offset = 0, size_type count = npos) const;

  // Description:
  // Swap the sequences stored by two strings.
  void swap(vtkUnicodeString&);

private:
  std::string Storage;
  class back_insert_iterator;
};

VTKCOMMONCORE_EXPORT bool operator==(const vtkUnicodeString& lhs, const vtkUnicodeString& rhs);
VTKCOMMONCORE_EXPORT bool operator!=(const vtkUnicodeString& lhs, const vtkUnicodeString& rhs);
VTKCOMMONCORE_EXPORT bool operator<(const vtkUnicodeString& lhs, const vtkUnicodeString& rhs);
VTKCOMMONCORE_EXPORT bool operator<=(const vtkUnicodeString& lhs, const vtkUnicodeString& rhs);
VTKCOMMONCORE_EXPORT bool operator>=(const vtkUnicodeString& lhs, const vtkUnicodeString& rhs);
VTKCOMMONCORE_EXPORT bool operator>(const vtkUnicodeString& lhs, const vtkUnicodeString& rhs);

#endif

// VTK-HeaderTest-Exclude: vtkUnicodeString.h
