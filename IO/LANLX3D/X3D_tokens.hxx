// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) 2021, Los Alamos National Laboratory
// SPDX-FileCopyrightText: Copyright © 2021. Triad National Security, LLC
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-LANL-Triad-USGov
/**
   \file X3D_tokens.hxx

   Implement FORTRAN data and control descriptors as C++ classes
   suitable for use with C++ streams.

   The five parameterized descriptors correspond to the tokens
   (terminal symbols) of the FLAG X3D file format; use of these
   descriptors according to the formats specified in "Summary of the
   FLAG X3D Format" (LA-UR-04-9033 V. 1.2) constitutes a scanner for
   the X3D format.

   \author Mark G. Gray <gray@lanl.gov>
*/

#ifndef X3D_TOKENS_HXX
#define X3D_TOKENS_HXX

#include "vtkABINamespace.h"

#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>

namespace X3D
{
VTK_ABI_NAMESPACE_BEGIN
/**
   \class ScanError

   Exception thrown by Aformat, Iformat, PEformat, Rformat, and Xformat.

   When Aformat, Iformat, PEformat, Rformat, or Xformat encounters
   an unexpected token in the stream being read, it throws this
   exception with a message containing the token and character
   position in the stream where found.
*/
class ScanError : public std::runtime_error
{
public:
  explicit ScanError(const std::string& m)
    : std::runtime_error(m.c_str())
  {
  }
  explicit ScanError(std::string unexpect, std::streamoff where)
    : std::runtime_error((unexpect + std::to_string(where)).c_str())
  {
  }
};

/**
    Read and ignore rest of line including newline.

    If the last line does not terminate with a newline,
    this will read through the end of file, setting stream
    internal state flags eofbit and failbit.

    \param is lhs of >>
    \return stream reference
    \exception ScanError if EOF reached before next newline.
*/
inline std::istream& eat_endl(std::istream& is)
{
  char c;
  while (is.get(c) && c != '\n')
    ;
  if (is.eof())
    throw ScanError("Unexpected EOF at character: ", is.tellg());
  return is;
}

/**
   Get width characters or until newline from istream,
   return in whitespace trimmed string.

   If width=0, return next whitespace terminated string.
   End get at newline.

   \param is stream to read from
   \param width number of characters to read
   \return string whitespace trimmed string containing read characters
   \exception ScanError if newline reached before width characters
*/
extern std::string fixed_get(std::istream& is, unsigned int width);

/**
   \class Aformat

   Fortran CHARACTER data descriptor A.
*/
class Aformat
{
public:
  /**
     Construct format descriptor

     Default width skips whitespace and
     reads next whitespace delimited string.

     \param w field width
  */
  explicit Aformat(unsigned int w = 0)
    : width(w)
  {
  }

  /**
     Set format width.

     Return reference to *this for chaining.
     Use width=0 to print as many digits as needed.

     \param w field width
     \return Aformat reference
  */
  Aformat& setw(unsigned int w)
  {
    width = w;
    return *this;
  }

  /**
     Set value and return string in Aw formatted string.

     \param s string to assign
     \return string containing formatted s
  **/
  std::string operator()(std::string s);

  /**
     Extract characters from Aw read of stream.

     End extract at newline.

     \param is lhs of >>
     \param a rhs of >>
     \return istream reference
     \exception ScanError if characters on line < width.
  **/
  friend std::istream& operator>>(std::istream& is, Aformat& a);

  /**
     Return value from last get.

     \return value
  */
  std::string operator()() const { return value; }

private:
  unsigned int width; // of field
  std::string value;
};

/**
   Extract trimmed string from Aw read of stream.

   End extract at newline.

   \param is lhs of >>
   \param a rhs of <<
   \return istream reference
*/
inline std::istream& operator>>(std::istream& is, Aformat& a)
{
  a.value = fixed_get(is, a.width);
  return is;
}

/**
   \class Iformat

   Fortran INTEGER data descriptor I.
**/
class Iformat
{
public:
  /**
     Construct format descriptor

     Default width skips whitespace and
     reads next non-digit terminated integer.

     \param w field width
  */
  explicit Iformat(unsigned int w = 0)
    : width(w)
    , value(0)
  {
  }

  /**
     Set format width.

     Return reference to *this for chaining.
     Use width=0 to print as many digits as needed.

     \param w field width
     \return Iformat reference
  */
  Iformat& setw(unsigned int w)
  {
    width = w;
    return *this;
  }

  /**
      Set value and return integer in Iw formatted string.

      \param i value to format
      \return string containing formatted value
  **/
  std::string operator()(int i);

  friend std::istream& operator>>(std::istream& is, Iformat& i);

  /**
     Return value from last get.

     \return value
  */
  int operator()() const { return value; }

private:
  unsigned int width; // of field
  int value;
};

/**
   Extract integer from Iw read of stream.

   End extract at newline.

   \param is lhs of >>
   \param i rhs of >>
   \return istream reference
   \exception ScanError if characters on line < width or
                              cannot convert token to integer.
**/
inline std::istream& operator>>(std::istream& is, Iformat& i)
{
  std::string s(fixed_get(is, i.width));
  try
  {
    i.value = std::stoi(s);
  }
  catch (const std::invalid_argument&)
  {
    throw ScanError("Cannot convert \"" + s + "\" to int before: ", is.tellg());
  }
  catch (const std::out_of_range&)
  {
    throw ScanError("Token \"" + s + "\" out of int range before: ", is.tellg());
  }
  return is;
}

/**
   \class PEformat

   Fortran REAL data descriptor 1PE.
*/
class PEformat
{
public:
  /**
     Construct format descriptor.

     Default width and precision matches C++ default for doubles.

     \param w field width
     \param p field precision
  */
  explicit PEformat(unsigned int w = 0, unsigned int d = 6)
    : width(w)
    , precision(d)
    , value(0.0)
  {
  }

  /**
     Set format width.

     Return reference to *this for chaining.
     Use width=0 to print as many digits as needed.

     \param w field width
     \return PEformat reference
  */
  PEformat& setw(unsigned int w)
  {
    width = w;
    return *this;
  }

  /**
     Set format precision.

     Return reference to *this for chaining.

     \param p field precision
     \return PEformat reference
  */
  PEformat& setprecision(unsigned int d)
  {
    precision = d;
    return *this;
  }

  friend std::istream& operator>>(std::istream& is, PEformat& pe);

  /**
     Return value from last get.

     \return double
  */
  double operator()() const { return value; }

  /**
     Set value and return double in 1PEw.d formatted string.

     \param x value to format
     \return string containing formatted value
  */
  std::string operator()(double x);

private:
  unsigned int width;     // of field
  unsigned int precision; // of value
  double value;
};

/**
   Extract double from 1PEw.d read of stream.

   End extract at newline.

   \param is lhs of >>
   \param pe rhs of >>
   \return istream reference
   \exception ScanError if characters on line < width or
                              cannot convert token to double.
*/
inline std::istream& operator>>(std::istream& is, PEformat& pe)
{
  std::string s(fixed_get(is, pe.width));
  try
  {
    pe.value = std::stod(s);
  }
  catch (const std::invalid_argument&)
  {
    throw ScanError("Cannot convert \"" + s + "\" to double before: ", is.tellg());
  }
  catch (const std::out_of_range&)
  {
    throw ScanError("Token \"" + s + "\" overflows double before: ", is.tellg());
  }
  return is;
}

/**
   \class Rformat

   Non-Fortran control descriptor for periodic end of line.
*/
class Rformat
{
public:
  /**
   Construct descriptor.

   Count use and put endl or get eat_endl with frequency n.
   If n=0 (the default), never put endl or get eat_endl.
  */
  Rformat(int n = 0)
    : count(n)
    , counter_(0)
  {
  }
  int operator()() const { return counter_; }
  void reset() { counter_ = 0; }
  friend std::istream& operator>>(std::istream& is, Rformat& r);
  friend std::ostream& operator<<(std::ostream& os, Rformat& r);

private:
  int count;
  int counter_;
};

/**
   \function Zmod

   Return representation of i in Z/n.  N.B. Z/0 == Z.

   \param n quotient group divisor
   \param i member of Z
   \return member of Z/n representing i.
*/
inline int Zmod(int n, int i)
{
  return n ? i % n : i;
}

inline std::istream& operator>>(std::istream& is, Rformat& r)
{
  r.counter_ = Zmod(r.count, r.counter_ + 1);
  if (r.counter_ != 0)
  {
    return is;
  }
  else
  {
    return is >> eat_endl;
  }
}

inline std::ostream& operator<<(std::ostream& os, Rformat& r)
{
  r.counter_ = Zmod(r.count, r.counter_ + 1);
  if (r.counter_ != 0)
  {
    return os;
  }
  else
  {
    return os << std::endl;
  }
}

/**
   \class Xformat

   Fortran control descriptor X.
*/
class Xformat
{
public:
  /**
     Construct format descriptor

     Default width discards next character.

     \param w field width
  */
  explicit Xformat(unsigned int w = 1)
    : width(w)
  {
  }

  /**
     Set format width.

     Return reference to *this for chaining.

     \param w field width
     \return Xformat reference
  */
  Xformat& setw(unsigned int w)
  {
    width = w;
    return *this;
  }

  /**
     Set width and return blanks in wX formatted string.

     \param w field width
     \return string of width blanks
  */
  std::string operator()(unsigned int w)
  {
    width = w;
    return std::string(width, ' ');
  }

  /**
     Return blanks in wX formatted string.

     \return string of width blanks
  */
  std::string operator()() const { return std::string(width, ' '); }

  friend std::istream& operator>>(std::istream& is, Xformat& x);

private:
  unsigned int width; // of field
};
/**
   Extract and ignore characters from wX read of stream.

   End extract at newline.

   \param is lhs of >>
   \param x rhs of >>
   \return istream reference
*/
inline std::istream& operator>>(std::istream& is, Xformat& x)
{
  fixed_get(is, x.width);
  return is;
}
VTK_ABI_NAMESPACE_END
}
#endif
