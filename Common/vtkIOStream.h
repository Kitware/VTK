/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIOStream.h
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
// .NAME vtkIOStream - Include C++ iostreams as used by VTK.
// .SECTION Description
// VTK supports both ANSI and old-style streams.  This header includes
// the proper streams according to VTK_USE_ANSI_STDLIB.

#ifndef __vtkIOStream_h
#define __vtkIOStream_h

#include "vtkConfigure.h"

#ifdef VTK_USE_ANSI_STDLIB

# include <iostream>  // Include real ansi istream and ostream.
# include <strstream> // Include real ansi strstreams.
# include <fstream>   // Include real ansi ifstream and ofstream.
# include <iomanip>   // Include real ansi io manipulators.

// Need these in global namespace so the same code will work with ansi
// and old-style streams.
using std::dec;
using std::hex;
using std::setw;
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

#else

// Include old-style streams.
# ifdef _WIN32_WCE
#  include "vtkWinCE.h"   // Include mini-streams for Windows CE.
# else
#  include <iostream.h>   // Include old-style istream and ostream.
#  if defined(_MSC_VER)
#   include <strstrea.h>  // Include old-style strstream from MSVC.
#  else
#   include <strstream.h> // Include old-style strstream.
#  endif
#  include <fstream.h>    // Include old-style ifstream and ofstream.
# endif
#endif

#endif // __vtkIOStream_h
