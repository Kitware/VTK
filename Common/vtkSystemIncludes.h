/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSystemIncludes.h
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
// .NAME vtkSystemIncludes - transition VTK to ANSI C++, centralize inclusion of system files
// .SECTION Description
// The vtkSystemIncludes centralizes the inclusion of system include
// files. (This is particularly important as VTK moves towards ANSI C++.)
// For example, this include file enables user's to build VTK with STL (i.e.,
// use std::ostream and other standard ANSI C++ functionality).  A
// compile-time flag (VTK_USE_ANSI_STDLIB) must be set to enable ANSI C++
// compliance. You'll probably also need to set various compiler flags.
// For example, on WIndows for ANSI C++, use 
// /D "VTK_USE_ANSI_STDLIB" /GX /Zm1000 .

#ifndef __vtkSystemIncludes_h
#define __vtkSystemIncludes_h

/* first include the local configuration for this machine */
#include "vtkConfigure.h"

// include  generic stuff 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Handle changes for ANSI C++ ---------------------------------------------
#ifdef VTK_USE_ANSI_STDLIB
#include <iostream>
#include <strstream>
#include <fstream>
using std::cerr;
using std::cout;
using std::cin;
using std::ios;
using std::endl;
using std::ends;
using std::ostream;
using std::istream;
using std::ostrstream;
using std::istrstream;
using std::strstream;
using std::ofstream;
using std::ifstream;
using std::fstream;

// otherwise, non-ANSI -----------------------------------------------------
#else
#ifdef _WIN32_WCE
  #include "vtkWinCE.h"
#else
  #include <iostream.h>
  #if defined(_MSC_VER)
    #include <strstrea.h>
  #else
    #include <strstream.h>
  #endif
  #include <fstream.h>
#endif // Win CE
#endif 

#define VTK_HAS_ID_TYPE
#ifdef VTK_USE_64BIT_IDS
#ifdef _WIN32
typedef __int64 vtkIdType;

/* ostream operator for __int64 */
inline ostream& __cdecl operator<<(ostream& _O, __int64 i64Val)
{
  wchar_t wchBuf[32];
  if (i64Val < 0)
  {
    _O << char("-");
    i64Val *= -1;
  }
  return (_O << _i64tow(i64Val, wchBuf, 10));
};

#else
typedef long long vtkIdType;
#endif
#else
typedef int vtkIdType;
#endif

#endif

