/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTexturedSphereSource.h
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
// .NAME vtkTexturedSphereSource - create a sphere centered at the origin
// .SECTION Description
// vtkTexturedSphereSource creates a polygonal sphere of specified radius 
// centered at the origin. The resolution (polygonal discretization) in both 
// the latitude (phi) and longitude (theta) directions can be specified. 
// It also is possible to create partial sphere by specifying maximum phi and 
// theta angles.

#ifndef __vtkTexturedSphereSource_h
#define __vtkTexturedSphereSource_h

#include "vtkPolyDataSource.h"

#define VTK_MAX_SPHERE_RESOLUTION 1024

class VTK_GRAPHICS_EXPORT vtkTexturedSphereSource : public vtkPolyDataSource 
{
public:
  vtkTypeMacro(vtkTexturedSphereSource,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct sphere with radius=0.5 and default resolution 8 in both Phi
  // and Theta directions.
  static vtkTexturedSphereSource *New();

  // Description:
  // Set radius of sphere.
  vtkSetClampMacro(Radius,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(Radius,float);

  // Description:
  // Set the number of points in the longitude direction.
  vtkSetClampMacro(ThetaResolution,int,4,VTK_MAX_SPHERE_RESOLUTION);
  vtkGetMacro(ThetaResolution,int);

  // Description:
  // Set the number of points in the latitude direction.
  vtkSetClampMacro(PhiResolution,int,4,VTK_MAX_SPHERE_RESOLUTION);
  vtkGetMacro(PhiResolution,int);

  // Description:
  // Set the maximum longitude angle.
  vtkSetClampMacro(Theta,float,0.0,360.0);
  vtkGetMacro(Theta,float);

  // Description:
  // Set the maximum latitude angle (0 is at north pole).
  vtkSetClampMacro(Phi,float,0.0,180.0);
  vtkGetMacro(Phi,float);

protected:
  vtkTexturedSphereSource(int res=8);
  ~vtkTexturedSphereSource() {};

  void Execute();
  float Radius;
  float Theta;
  float Phi;
  int ThetaResolution;
  int PhiResolution;

private:
  vtkTexturedSphereSource(const vtkTexturedSphereSource&);  // Not implemented.
  void operator=(const vtkTexturedSphereSource&);  // Not implemented.
};

#endif


