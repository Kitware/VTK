/*=========================================================================

  Program:   Visualization Toolkit
  Module:    %M%
  Language:  C++
  Date:      %D%
  Version:   %V%

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
// .NAME vtkWindowLevelLookupTable - map scalar values into colors or colors to scalars; generate color table
// .SECTION Description
// vtkWindowLevelLookupTable is an object that is used by mapper objects
// to map scalar values into rgba (red-green-blue-alpha transparency)
// color specification, or rgba into scalar values. The color table can
// be created by direct insertion of color values, or by specifying a
// window and level. Window / Level is used in medical imaging to specify
// a linear greyscale ramp. The Level is the center of the ramp.  The
// Window is the width of the ramp.

// .SECTION Caveats
// vtkWindowLevelLookupTable is a reference counted object. Therefore, you
// should always use operator "new" to construct new objects. This procedure
// will avoid memory problems (see text).

// .SECTION See Also
// vtkLogLookupTable

#ifndef __vtkWindowLevelLookupTable_h
#define __vtkWindowLevelLookupTable_h

#include "vtkObject.h"
#include "vtkLookupTable.h"

class VTK_EXPORT vtkWindowLevelLookupTable : public vtkLookupTable
{
public:
  static vtkWindowLevelLookupTable *New();
  vtkTypeMacro(vtkWindowLevelLookupTable,vtkLookupTable);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Generate lookup table from window and level.
  // Table is built as a linear ramp, centered at Level and of width Window.
  void Build();

  // Description:
  // Set the Window for the lookup table. Window is the width of the
  // lookup table ramp.
  vtkSetClampMacro(Window,float,1.0,65536.0);
  vtkGetMacro(Window,float);

  // Description:
  // Set the Level for the lookup table. Level is the center of the ramp of
  // the lookup table.
  vtkSetMacro(Level,float);
  vtkGetMacro(Level,float);

  // Description:
  // Set inverse video on or off.
  vtkSetMacro(InverseVideo,int);
  vtkGetMacro(InverseVideo,int);
  vtkBooleanMacro(InverseVideo,int);

  // Description:
  // Set the Minimum color. All lookup table entries below the start of the ramp
  // will be set to this color.
  vtkSetVector4Macro(MinimumColor,unsigned char);
  vtkGetVectorMacro(MinimumColor,unsigned char,4);

  // Description:
  // Set the Maximum color. All lookup table entries above the end of the ramp
  // will be set to this color.
  vtkSetVector4Macro(MaximumColor,unsigned char);
  vtkGetVectorMacro(MaximumColor,unsigned char,4);

protected:
  vtkWindowLevelLookupTable(int sze=256, int ext=256);
  ~vtkWindowLevelLookupTable() {};
  vtkWindowLevelLookupTable(const vtkWindowLevelLookupTable&) {};
  void operator=(const vtkWindowLevelLookupTable&) {};

  float Window;
  float Level;
  int MapScalarToIndex (float scalar);
  int InverseVideo;
  unsigned char MinimumColor[4];
  unsigned char MaximumColor[4];
};

#endif


