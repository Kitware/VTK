/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrowSource.h
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
// .NAME vtkArrowSource - Appends a cylinder/line to a cone to form an arrow.
// .SECTION Description
// vtkArrowSource was intended to be used as the source for a glyph.
// The shaft base is always at (0,0,0). The arrow tip is always at (1,0,0).
// The resolution of the cone and shaft can be set and default to 6.
// The radius of the cone and shaft can be set and default to 0.03 and 0.1.
// The length of the tip can also be set, and defaults to 0.35.


#ifndef __vtkArrowSource_h
#define __vtkArrowSource_h

#include "vtkPolyDataSource.h"

class VTK_GRAPHICS_EXPORT vtkArrowSource : public vtkPolyDataSource
{
public:
  // Description
  // Construct cone with angle of 45 degrees.
  static vtkArrowSource *New();

  vtkTypeMacro(vtkArrowSource,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Set the length, and radius of the tip.  They default to 0.35 and 0.1
  vtkSetClampMacro(TipLength,float,0.0,1.0);
  vtkGetMacro(TipLength,float);
  vtkSetClampMacro(TipRadius,float,0.0,10.0);
  vtkGetMacro(TipRadius,float);
  
  // Description:
  // Set the resolution of the tip.  The tip behaves the same as a cone.
  // Resoultion 1 gives a single triangle, 2 gives two crossed triangles.
  vtkSetClampMacro(TipResolution,int,1,128);
  vtkGetMacro(TipResolution,int);

  // Description:
  // Set the radius of the shaft.  Defaults to 0.03.
  vtkSetClampMacro(ShaftRadius,float,0.0,5.0);
  vtkGetMacro(ShaftRadius,float);

  // Description:
  // Set the resolution of the shaft.  0 gives a line, 1 gives a rectangle.
  vtkSetClampMacro(ShaftResolution,int,0,128);
  vtkGetMacro(ShaftResolution,int);

protected:
  vtkArrowSource();
  ~vtkArrowSource() {};
  vtkArrowSource(const vtkArrowSource&) {};
  void operator=(const vtkArrowSource&) {};

  void Execute();

  int TipResolution;
  float TipLength;
  float TipRadius;

  int ShaftResolution;
  float ShaftRadius;

};

#endif


