/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIOStream.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkConfigure.h"

// Hack access to some private stream implementations.
#if defined(VTK_IOSTREAM_NEED_OPERATORS_LL)
# ifdef _MSC_VER
#  pragma warning (push, 1)
# endif
# if defined(VTK_USE_ANSI_STDLIB)
#  if defined(_MSC_VER)
#   include <cerrno>
#   include <climits>
#   include <cstdio>
#   include <cstdlib>
#   include <xiosbase>
#   define private public
#   if defined(VTK_BUILD_SHARED_LIBS)
     // Avoid dllimport of num_get::_Getifld
#    undef _DLL
#    include <xlocnum>
#    define _DLL 1
#   else
#    include <xlocnum>
#   endif
#   undef private
#  else
#   error "The ANSI streams library does not support long long"
#  endif
#  include <iostream>
   using std::istream;
   using std::ostream;
# else
#  include <string.h> // memchr
#  include <stdio.h> // sscanf, sprintf
#  if defined(__HP_aCC)
#   define protected public
#   include <iostream.h>
#   undef protected
#  else
#   include <iostream.h>
#  endif
# endif
# ifdef _MSC_VER
#  pragma warning (pop)
# endif
#endif

// Include function declarations.
#include "vtkIOStream.h"

#if defined(VTK_IOSTREAM_NEED_OPERATORS_LL)

# if defined(VTK_USE_ANSI_STDLIB)

#  if defined(_MSC_VER)
// Template to scan an input stream for a __int64 or unsigned __int64 value.
template <class T>
std::istream& vtkIOStreamScanTemplate(std::istream& is, T& value, char type)
{
  std::ios_base::iostate state = std::ios_base::goodbit;
  const std::istream::sentry okay(is);
  if(okay)
    {
    try
      {
      // Copy the string to a buffer and construct the format string.
      std::istream::_Iter first = is.rdbuf();
      std::istream::_Iter last = 0;
      char buffer[_MAX_INT_DIG];
      char format[] = "%I64_";
      switch(std::istream::_Nget::_Getifld(buffer, first, last,
                                           is.flags(), is.getloc()))
        {
        case 8: format[4] = 'o'; break;
        case 10: format[4] = type; break;
        case 16: format[4] = 'x'; break;
        };

      // Use sscanf to parse the number from the buffer.
      T result;
      int success = (sscanf(buffer, format, &result) == 1)?1:0;

      // Set flags for resulting state.
      if(first == last) { state |= std::ios_base::eofbit; }
      if(!success) { state |= std::ios_base::failbit; }
      else { value = result; }
      }
    catch(...) { is.setstate(std::ios_base::badbit, true); }
    }
  
  is.setstate(state);
  return is;  
}

// Template to print a __int64 or unsigned __int64 value to an output stream.
template <class T>
std::ostream& vtkIOStreamPrintTemplate(std::ostream& os, T value, char type)
{
  std::ios_base::iostate state = std::ios_base::goodbit;
  const std::ostream::sentry okay(os);
  if(okay)
    {
    try
      {
      // Construct the format string.
      char format[8];
      char* f = format;
      *f++ = '%';
      if(os.flags() & std::ios_base::showpos) { *f++ = '+'; }
      if(os.flags() & std::ios_base::showbase) { *f++ = '#'; }
      *f++ = 'I';
      *f++ = '6';
      *f++ = '4';
      std::ios_base::fmtflags bflags = os.flags() & std::ios_base::basefield;
      if(bflags == std::ios_base::oct) { *f++ = 'o'; }
      else if(bflags != std::ios_base::hex) { *f++ = type; }
      else if(os.flags() & std::ios_base::uppercase) { *f++ = 'X'; }
      else { *f++ = 'x'; }
      *f = '\0';

      // Use sprintf to print to a buffer and then write the
      // buffer to the stream.
      char buffer[2*_MAX_INT_DIG];
      if(std::ostream::_Nput::_Iput(os.rdbuf(), os, os.fill(), buffer,
                                    sprintf(buffer, format, value)).failed())
        {
        state |= std::ios_base::badbit;
        }
      }
    catch(...) { os.setstate(std::ios_base::badbit, true); }
    }
  
  os.setstate(state);
  return os;
}
#  endif

# else // begin implementation for VTK_USE_ANSI_STDLIB not defined

#  if defined(_MAX_INT_DIG)
#   define VTK_TYPE_INT64_MAX_DIG _MAX_INT_DIG
#  else
#   define VTK_TYPE_INT64_MAX_DIG 32
#  endif

#  if !defined(VTK_ISTREAM_SUPPORTS_LONG_LONG)
static int vtkIOStreamScanStream(istream& is, char* buffer)
{
  // Prepare to write to buffer.
  char* out = buffer;
  char* end = buffer + VTK_TYPE_INT64_MAX_DIG - 1;

  // Look for leading sign.
  if(is.peek() == '+') { *out++ = '+'; is.ignore(); }
  else if(is.peek() == '-') { *out++ = '-'; is.ignore(); }

  // Determine the base.  If not specified in the stream, try to
  // detect it from the input.  A leading 0x means hex, and a leading
  // 0 alone means octal.
  int base = 0;
  int flags = is.flags() & ios::basefield;
  if(flags == ios::oct) { base = 8; }
  else if(flags == ios::dec) { base = 10; }
  else if(flags == ios::hex) { base = 16; }
  bool foundDigit = false;
  bool foundNonZero = false;
  if(is.peek() == '0')
    {
    foundDigit = true;
    is.ignore();
    if((is.peek() == 'x' || is.peek() == 'X') && (base == 0 || base == 16))
      {
      base = 16;
      foundDigit = false;
      is.ignore();
      }
    else if (base == 0)
      {
      base = 8;
      }
    }
  
  // Determine the range of digits allowed for this number.
  const char* digits = "0123456789abcdefABCDEF";
  int maxDigitIndex = 10;
  if(base == 8)
    {
    maxDigitIndex = 8;
    }
  else if(base == 16)
    {
    maxDigitIndex = 10+6+6;
    }

  // Scan until an invalid digit is found.
  for(;is.peek() != EOF; is.ignore())
    {
    if(memchr(digits, *out = (char)is.peek(), maxDigitIndex) != 0)
      {
      if((foundNonZero || *out != '0') && out < end)
        {
        ++out;
        foundNonZero = true;
        }
      foundDigit = true;
      }
    else
      {
      break;
      }
    }

  // Correct the buffer contents for degenerate cases.
  if(foundDigit && !foundNonZero)
    {
    *out++ = '0';
    }
  else if (!foundDigit)
    {
    out = buffer;
    }
  
  // Terminate the string in the buffer.
  *out = '\0';

  return base;
}

// Scan an input stream for a vtkIOStreamSLL or vtkIOStreamULL value.
template <class T>
istream& vtkIOStreamScanTemplate(istream& is, T& value, char type)
{
  int state = ios::goodbit;

  // Skip leading whitespace.
  is.eatwhite();

  if(is)
    {
    // Copy the string to a buffer and construct the format string.
    char buffer[VTK_TYPE_INT64_MAX_DIG];
#  if defined(_MSC_VER)
    char format[] = "%I64_";
    const int typeIndex = 4;
#  else
    char format[] = "%ll_";
    const int typeIndex = 3;
#  endif
    switch(vtkIOStreamScanStream(is, buffer))
      {
      case 8: format[typeIndex] = 'o'; break;
      case 0: // Default to decimal if not told otherwise.
      case 10: format[typeIndex] = type; break;
      case 16: format[typeIndex] = 'x'; break;
      };

    // Use sscanf to parse the number from the buffer.
    T result;
    int success = (sscanf(buffer, format, &result) == 1)?1:0;
    
    // Set flags for resulting state.
    if(is.peek() == EOF) { state |= ios::eofbit; }
    if(!success) { state |= ios::failbit; }
    else { value = result; }
    }
  
  is.clear(state);
  return is;  
}
#  endif

#  if !defined(VTK_OSTREAM_SUPPORTS_LONG_LONG)
// Print a vtkIOStreamSLL or vtkIOStreamULL value to an output stream.
template <class T>
ostream& vtkIOStreamPrintTemplate(ostream& os, T value, char type)
{
  if(os)
    {
    // Construct the format string.
    char format[8];
    char* f = format;
    *f++ = '%';
    if(os.flags() & ios::showpos) { *f++ = '+'; }
    if(os.flags() & ios::showbase) { *f++ = '#'; }
#  if defined(_MSC_VER)
    *f++ = 'I'; *f++ = '6'; *f++ = '4';
#  else
    *f++ = 'l'; *f++ = 'l';
#  endif
    long bflags = os.flags() & ios::basefield;
    if(bflags == ios::oct) { *f++ = 'o'; }
    else if(bflags != ios::hex) { *f++ = type; }
    else if(os.flags() & ios::uppercase) { *f++ = 'X'; }
    else { *f++ = 'x'; }
    *f = '\0';

    // Use sprintf to print to a buffer and then write the
    // buffer to the stream.
    char buffer[2*VTK_TYPE_INT64_MAX_DIG];
    sprintf(buffer, format, value);
    os << buffer;
    }
  return os;
}
#  endif
# endif // end implementation for VTK_USE_ANSI_STDLIB not defined

# if !defined(VTK_ISTREAM_SUPPORTS_LONG_LONG)
// Implement input stream operator for vtkIOStreamSLL.
istream& vtkIOStreamScan(istream& is, vtkIOStreamSLL& value)
{
  return vtkIOStreamScanTemplate(is, value, 'd');
}

// Implement input stream operator for vtkIOStreamULL.
istream& vtkIOStreamScan(istream& is, vtkIOStreamULL& value)
{
  return vtkIOStreamScanTemplate(is, value, 'u');
}
#endif

# if !defined(VTK_OSTREAM_SUPPORTS_LONG_LONG)
// Implement output stream operator for vtkIOStreamSLL.
ostream& vtkIOStreamPrint(ostream& os, vtkIOStreamSLL value)
{
  return vtkIOStreamPrintTemplate(os, value, 'd');
}

// Implement output stream operator for vtkIOStreamULL.
ostream& vtkIOStreamPrint(ostream& os, vtkIOStreamULL value)
{
  return vtkIOStreamPrintTemplate(os, value, 'u');
}
# endif

#endif
