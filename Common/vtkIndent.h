/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIndent.h
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

// .NAME vtkIndent - a simple class to control print indentation
// .SECTION Description
// vtkIndent is used to control indentation during the chaining print 
// process. This way nested objects can correctly indent themselves.

#ifndef __vtkIndent_h
#define __vtkIndent_h

#include "vtkWin32Header.h"
#include "vtkSystemIncludes.h"

class VTK_COMMON_EXPORT vtkIndent
{
public:
  void Delete() {delete this;};
  vtkIndent(int ind=0) {this->Indent=ind;};
  static vtkIndent *New();

  virtual const char *GetClassName() {return "vtkIndent";};

  // Description:
  // Determine the next indentation level. Keep indenting by two until the 
  // max of forty.
  vtkIndent GetNextIndent();

  //BTX
  // Description:
  // Print out the indentation. Basically output a bunch of spaces.
  friend VTK_COMMON_EXPORT ostream& operator<<(ostream& os, vtkIndent& o); 
  //ETX

protected:
  int Indent;
  
};

#endif
