/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCutMaterial.h
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
// .NAME vtkCutMaterial - Automatically computes the cut plane for a meterial array pair.
// .SECTION Description
// vtkCutMaterial computes a cut plane based on an up vector, center of the bounding box
// and the location of the maximum variable value.
//  These computed values are available so that they can be used to set the camera
// for the best view of the plane.


#ifndef __vtkCutMaterial_h
#define __vtkCutMaterial_h

#include "vtkDataSetToPolyDataFilter.h"
#include "vtkPlane.h"


class VTK_PARALLEL_EXPORT vtkCutMaterial : public vtkDataSetToPolyDataFilter
{
public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeMacro(vtkCutMaterial,vtkDataSetToPolyDataFilter);
  static vtkCutMaterial *New();

  // Description:
  // Cell array that contains the material values.
  vtkSetStringMacro(MaterialArrayName);
  vtkGetStringMacro(MaterialArrayName);
  
  // Description:
  // Material to probe.
  vtkSetMacro(Material, int);
  vtkGetMacro(Material, int);
  
  // Description:
  // For now, we just use the call values.
  // The array name to cut.
  vtkSetStringMacro(ArrayName);
  vtkGetStringMacro(ArrayName);

  // Description:
  // The last piece of information that specifies the plane.
  vtkSetVector3Macro(UpVector, float);
  vtkGetVector3Macro(UpVector, float);
  
  // Description:
  // Accesses to the values computed during the execute method.  They
  // could be used to get a good camera view for the resulting plane.
  vtkGetVector3Macro(MaximumPoint, float);
  vtkGetVector3Macro(CenterPoint, float);
  vtkGetVector3Macro(Normal, float);
  
protected:
  vtkCutMaterial();
  ~vtkCutMaterial();

  void Execute(); //generate output data
  void ComputeMaximumPoint(vtkDataSet *input);
  void ComputeNormal();

  char *MaterialArrayName;
  int Material;
  char *ArrayName;
  float UpVector[3];
  float MaximumPoint[3];
  float CenterPoint[3];
  float Normal[3];
  
  vtkPlane *PlaneFunction;
  
private:
  vtkCutMaterial(const vtkCutMaterial&);  // Not implemented.
  void operator=(const vtkCutMaterial&);  // Not implemented.
};

#endif
