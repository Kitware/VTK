/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSuperquadricSource.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Mike Halle, Brigham and Women's Hospital


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
// .NAME vtkSuperquadricSource - create a polygonal superquadric centered 
// at the origin
// .SECTION Description
// vtkSuperquadricSource creates a superquadric (represented by polygons) 
// of specified
// size centered at the origin. The resolution (polygonal discretization)
// in both the latitude (phi) and longitude (theta) directions can be
// specified. Roundness parameters (PhiRoundness and ThetaRoundness) control
// the shape of the superquadric.  The Toroidal boolean controls whether
// a toroidal superquadric is produced.  If so, the Thickness parameter
// controls the thickness of the toroid:  0 is the thinnest allowable
// toroid, and 1 has a minimum sized hole.  The Scale parameters allow 
// the superquadric to be scaled in x, y, and z (normal vectors are correctly
// generated in any case).  The Size parameter controls size of the 
// superquadric.
//
// This code is based on "Rigid physically based superquadrics", A. H. Barr,
// in "Graphics Gems III", David Kirk, ed., Academic Press, 1992.
//
// .SECTION Caveats
// Resolution means the number of latitude or longitude lines for a complete 
// superquadric. The resolution parameters are rounded to the nearest 4
// in phi and 8 in theta.  
//
// Texture coordinates are not equally distributed around all superquadrics.
// 
// The Size and Thickness parameters control coefficients of superquadric
// generation, and may do not exactly describe the size of the superquadric.
//

#ifndef __vtkSuperquadricSource_h
#define __vtkSuperquadricSource_h

#include "vtkPolyDataSource.h"

#define VTK_MAX_SUPERQUADRIC_RESOLUTION 1024
#define VTK_MIN_SUPERQUADRIC_THICKNESS  1e-4
#define VTK_MIN_SUPERQUADRIC_ROUNDNESS  1e-24

class VTK_GRAPHICS_EXPORT vtkSuperquadricSource : public vtkPolyDataSource 
{
public:
  // Description:
  // Create a default superquadric with a radius of 0.5, non-toroidal, 
  // spherical, and centered at the origin.
  static vtkSuperquadricSource *New();

  vtkTypeMacro(vtkSuperquadricSource,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the center of the superquadric. Default is 0,0,0.
  vtkSetVector3Macro(Center,float);
  vtkGetVectorMacro(Center,float,3);

  // Description:
  // Set the scale factors of the superquadric. Default is 1,1,1.
  vtkSetVector3Macro(Scale,float);
  vtkGetVectorMacro(Scale,float,3);

  // Description:
  // Set the number of points in the longitude direction.
  vtkGetMacro(ThetaResolution,int);
  void SetThetaResolution(int i);

  // Description:
  // Set the number of points in the latitude direction.
  vtkGetMacro(PhiResolution,int);
  void SetPhiResolution(int i);

  // Description:
  // Set/Get Superquadric ring thickness (toroids only).
  // Changing thickness maintains the outside diameter of the toroid.
  vtkGetMacro(Thickness,float);
  vtkSetClampMacro(Thickness,float,VTK_MIN_SUPERQUADRIC_THICKNESS,1.0);

  // Description:
  // Set/Get Superquadric north/south roundness. 
  // Values range from 0 (rectangular) to 1 (circular) to higher orders.
  vtkGetMacro(PhiRoundness,float);
  void SetPhiRoundness(float e); 

  // Description:
  // Set/Get Superquadric east/west roundness.
  // Values range from 0 (rectangular) to 1 (circular) to higher orders.
  vtkGetMacro(ThetaRoundness,float);
  void SetThetaRoundness(float e);

  // Description:
  // Set/Get Superquadric isotropic size.
  vtkSetMacro(Size,float);
  vtkGetMacro(Size,float);

  // Description:
  // Set/Get whether or not the superquadric is toroidal (1) or ellipsoidal (0).
  vtkBooleanMacro(Toroidal,int);
  vtkGetMacro(Toroidal,int);
  vtkSetMacro(Toroidal,int);

protected:
  vtkSuperquadricSource(int res=16);
  ~vtkSuperquadricSource() {};

  int Toroidal;
  float Thickness;
  float Size;
  float PhiRoundness;
  float ThetaRoundness;
  void Execute();
  float Center[3];
  float Scale[3];
  int ThetaResolution;
  int PhiResolution;

private:
  vtkSuperquadricSource(const vtkSuperquadricSource&);  // Not implemented.
  void operator=(const vtkSuperquadricSource&);  // Not implemented.
};

#endif


