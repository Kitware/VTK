/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStdString.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkStdString - Wrapper around vtkstd::string to keep symbols short.
// .SECTION Description
// vtkStdString derives from vtkstd::string to provide shorter symbol
// names than std::basic_string<...> given by the standard STL string.

#ifndef __vtkStdString_h
#define __vtkStdString_h

#include "vtkSystemIncludes.h" // For VTK_COMMON_EXPORT.
#include <vtkstd/string>       // For the superclass.

class vtkStdString;
VTK_COMMON_EXPORT ostream& operator<<(ostream&, const vtkStdString&);

class vtkStdString : public vtkstd::string
{
public:
  typedef vtkstd::string StdString;
  typedef StdString::value_type             value_type;
  typedef StdString::pointer                pointer;
  typedef StdString::reference              reference;
  typedef StdString::const_reference        const_reference;
  typedef StdString::size_type              size_type;
  typedef StdString::difference_type        difference_type;
  typedef StdString::iterator               iterator;
  typedef StdString::const_iterator         const_iterator;
  typedef StdString::reverse_iterator       reverse_iterator;
  typedef StdString::const_reverse_iterator const_reverse_iterator;

  vtkStdString(): StdString() {}
  vtkStdString(const value_type* s): StdString(s) {}
  vtkStdString(const value_type* s, size_type n): StdString(s, n) {}
  vtkStdString(const StdString& s, size_type pos=0, size_type n=npos):
    StdString(s, pos, n) {}
};

#endif
