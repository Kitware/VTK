/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMassProperties.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Abdalmajeid M. Alyassin who developed this class.

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
// .NAME vtkMassProperties - estimate volume, area, shape index of triangle mesh
// .SECTION Description
// vtkMassProperties estimates the volume, the surface area, and the
// normalized shape index of a triangle mesh.  The algorithm
// implemented here is based on the discrete form of the divergence
// theorem.  The general assumption here is that the model is of
// closed surface.  For more details see the following reference
// (Alyassin A.M. et al, "Evaluation of new algorithms for the
// interactive measurement of surface area and volume", Med Phys 21(6)
// 1994.).  

// .SECTION Caveats
// Currently only triangles are processed. Use vtkTriangleFilter to
// convert any strips or polygons to triangles.

// .SECTION See Also
// vtkTriangleFilter

#ifndef __vtkMassProperties_h
#define __vtkMassProperties_h

#include "vtkProcessObject.h"
#include "vtkPolyData.h"

class VTK_EXPORT vtkMassProperties : public vtkProcessObject
{
public:
  // Description:
  // Constructs with initial values of zero.
  static vtkMassProperties *New();

  vtkTypeMacro(vtkMassProperties,vtkProcessObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Compute and return the volume.
  double GetVolume() {this->Update(); return this->Volume;}

  // Description:
  // Compute and return the volume projected on to each axis aligned plane.
  double GetVolumeX() {this->Update(); return this->VolumeX;}
  double GetVolumeY() {this->Update(); return this->VolumeY;}
  double GetVolumeZ() {this->Update(); return this->VolumeZ;}

  // Description:
  // Compute and return the weighting factors for the maximum unit
  // normal component (MUNC).
  double GetKx() {this->Update(); return this->Kx;}
  double GetKy() {this->Update(); return this->Ky;}
  double GetKz() {this->Update(); return this->Kz;}

  // Description:
  // Compute and return the area.
  double GetSurfaceArea() {this->Update(); return this->SurfaceArea;}

  // Description:
  // Compute and return the normalized shape index. This characterizes the
  // deviation of the shape of an object from a sphere. A sphere's NSI
  // is one. This number is always >= 1.0.
  double GetNormalizedShapeIndex() 
    {this->Update(); return this->NormalizedShapeIndex;}

  void Update();
  
  void SetInput(vtkPolyData *input);
  vtkPolyData *GetInput();

protected:
  vtkMassProperties();
  ~vtkMassProperties();
  vtkMassProperties(const vtkMassProperties&);
  void operator=(const vtkMassProperties&);

  void Execute();

  double  SurfaceArea;
  double  Volume;
  double  VolumeX;
  double  VolumeY;
  double  VolumeZ;
  double  Kx;
  double  Ky;
  double  Kz;
  double  NormalizedShapeIndex;
  vtkTimeStamp ExecuteTime;

};

#endif


