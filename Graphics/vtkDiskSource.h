/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDiskSource.h
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
// .NAME vtkDiskSource - create a disk with hole in center
// .SECTION Description
// vtkDiskSource creates a polygonal disk with a hole in the center. The 
// disk has zero height. The user can specify the inner and outer radius
// of the disk, and the radial and circumferential resolution of the 
// polygonal representation. 
// .SECTION See Also
// vtkLinearExtrusionFilter

#ifndef __vtkDiskSource_h
#define __vtkDiskSource_h

#include "vtkPolyDataSource.h"
#include "vtkPolyData.h"

class VTK_GRAPHICS_EXPORT vtkDiskSource : public vtkPolyDataSource 
{
public:
  static vtkDiskSource *New();
  vtkTypeMacro(vtkDiskSource,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify inner radius of hole in disc.
  vtkSetClampMacro(InnerRadius,float,0.0,VTK_LARGE_FLOAT)
  vtkGetMacro(InnerRadius,float);

  // Description:
  // Specify outer radius of disc.
  vtkSetClampMacro(OuterRadius,float,0.0,VTK_LARGE_FLOAT)
  vtkGetMacro(OuterRadius,float);

  // Description:
  // Set the number of points in radius direction.
  vtkSetClampMacro(RadialResolution,int,1,VTK_LARGE_INTEGER)
  vtkGetMacro(RadialResolution,int);

  // Description:
  // Set the number of points in circumferential direction.
  vtkSetClampMacro(CircumferentialResolution,int,3,VTK_LARGE_INTEGER)
  vtkGetMacro(CircumferentialResolution,int);

protected:
  vtkDiskSource();
  ~vtkDiskSource() {};

  void Execute();
  float InnerRadius;
  float OuterRadius;
  int RadialResolution;
  int CircumferentialResolution;

private:
  vtkDiskSource(const vtkDiskSource&);  // Not implemented.
  void operator=(const vtkDiskSource&);  // Not implemented.
};

#endif


