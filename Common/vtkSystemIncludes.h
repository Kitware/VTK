/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSystemIncludes.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
using std::ofstream;
using std::ifstream;

// otherwise, non-ANSI -----------------------------------------------------
#else
#include <iostream.h>
#if defined(_MSC_VER)
#include <strstrea.h>
#else
#include <strstream.h>
#endif
#include <fstream.h>
#endif

#ifdef VTK_USE_64BIT_IDS
typedef long vtkIdType;
#else
typedef int vtkIdType;
#endif

#endif
