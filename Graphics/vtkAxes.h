/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAxes.h
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
// .NAME vtkAxes - create an x-y-z axes
// .SECTION Description
// vtkAxes creates three lines that form an x-y-z axes. The origin of the
// axes is user specified (0,0,0 is default), and the size is specified with
// a scale factor. Three scalar values are generated for the three lines and
// can be used (via color map) to indicate a particular coordinate axis.

#ifndef __vtkAxes_h
#define __vtkAxes_h

#include "vtkPolyDataSource.h"

class VTK_GRAPHICS_EXPORT vtkAxes : public vtkPolyDataSource 
{
public:
  static vtkAxes *New();

  vtkTypeMacro(vtkAxes,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the origin of the axes.
  vtkSetVector3Macro(Origin,float);
  vtkGetVectorMacro(Origin,float,3);

  // Description:
  // Set the scale factor of the axes. Used to control size.
  vtkSetMacro(ScaleFactor,float);
  vtkGetMacro(ScaleFactor,float);

  // Description:
  // If Symetric is on, the the axis continue to negative values.
  vtkSetMacro(Symmetric,int);
  vtkGetMacro(Symmetric,int);
  vtkBooleanMacro(Symmetric,int);

  // Description:
  // Option for computing normals.  By default they are computed.
  vtkSetMacro(ComputeNormals, int);
  vtkGetMacro(ComputeNormals, int);
  vtkBooleanMacro(ComputeNormals, int);
  
protected:
  vtkAxes();
  ~vtkAxes() {};

  void Execute();
  // This source does not know how to generate pieces yet.
  int ComputeDivisionExtents(vtkDataObject *output, 
			     int idx, int numDivisions);

  float Origin[3];
  float ScaleFactor;
  
  int Symmetric;
  int ComputeNormals;
private:
  vtkAxes(const vtkAxes&);  // Not implemented.
  void operator=(const vtkAxes&);  // Not implemented.
};

#endif


