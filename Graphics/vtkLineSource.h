/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLineSource.h
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
// .NAME vtkLineSource - create a line defined by two end points
// .SECTION Description
// vtkLineSource is a source object that creates a polyline defined by
// two endpoints. The number of segments composing the polyline is
// controlled by setting the object resolution.

#ifndef __vtkLineSource_h
#define __vtkLineSource_h

#include "vtkPolyDataSource.h"

class VTK_EXPORT vtkLineSource : public vtkPolyDataSource 
{
public:
  static vtkLineSource *New();
  vtkTypeMacro(vtkLineSource,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set position of first end point.
  vtkSetVector3Macro(Point1,float);
  vtkGetVectorMacro(Point1,float,3);

  // Description:
  // Set position of other end point.
  vtkSetVector3Macro(Point2,float);
  vtkGetVectorMacro(Point2,float,3);

  // Description:
  // Divide line into resolution number of pieces.
  vtkSetClampMacro(Resolution,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(Resolution,int);

protected:
  vtkLineSource(int res=1);
  ~vtkLineSource() {};
  vtkLineSource(const vtkLineSource&);
  void operator=(const vtkLineSource&);

  void Execute();
  float Point1[3];
  float Point2[3];
  int Resolution;
};

#endif


