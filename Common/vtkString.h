/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkString.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkString - performs common string operations
// .SECTION Description
// vtkString is a collection of methods to perform common string operations. 
// The strings in this case are not some reference counted objects but
// the traditional c-strings (char*). This class provides platform 
// independent methods for creating, copying, ... of strings.

#ifndef __vtkString_h
#define __vtkString_h

#include "vtkObject.h"

class VTK_COMMON_EXPORT vtkString : public vtkObject
{
public:
  static vtkString *New();
  vtkTypeRevisionMacro(vtkString,vtkObject);

  // Description:
  // This method returns the size of string. If the string is empty,
  // it returns 0. It can handle null pointers.
  static vtkIdType Length(const char* str);

  // Description:
  // Copy string to the other string.
  static void Copy(char* dest, const char* src);

  // Description:
  // This method makes a duplicate of the string similar to
  // C function strdup but it uses new to create new string, so
  // you can use delete to remove it. It returns empty string 
  // "" if the input is empty.
  static char* Duplicate(const char* str);

  // Description:
  // This method compare two strings. It is similar to strcmp,
  // but it can handle null pointers.
  static int Compare(const char* str1, const char* str2);

  // Description:
  // This method compare two strings. It is similar to strcmp,
  // but it can handle null pointers. Also it only returns 
  // C style true or false versus compare which returns also which 
  // one is greater.
  static int Equals(const char* str1, const char* str2)
    { return vtkString::Compare(str1, str2) == 0; }

  // Description:
  // Check if the first string starts with the second one.
  static int StartsWith(const char* str1, const char* str2);

  // Description:
  // Check if the first string starts with the second one.
  static int EndsWith(const char* str1, const char* str2);
  
protected:
  vtkString() {};
  ~vtkString() {};

private:
  vtkString(const vtkString&);  // Not implemented.
  void operator=(const vtkString&);  // Not implemented.
};

#endif
