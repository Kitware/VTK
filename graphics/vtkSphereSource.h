/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSphereSource.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkSphereSource - create a polygonal sphere centered at the origin
// .SECTION Description
// vtkSphereSource creates a sphere (represented by polygons) of specified
// radius centered at the origin. The resolution (polygonal discretization)
// in both the latitude (phi) and longitude (theta) directions can be
// specified. It also is possible to create partial spheres by specifying
// maximum phi and theta angles.
// .SECTION Caveats
// Resolution means the number of latitude or longitude lines for a complete 
// sphere. If you create partial spheres the number of latitude/longitude 
// lines may be off by one. 

#ifndef __vtkSphereSource_h
#define __vtkSphereSource_h

#include "vtkPolyDataSource.h"

#define VTK_MAX_SPHERE_RESOLUTION 1024

class VTK_EXPORT vtkSphereSource : public vtkPolyDataSource 
{
public:

// Description:
// Construct sphere with radius=0.5 and default resolution 8 in both Phi
// and Theta directions. Theta ranges from (0,360) and phi (0,180) degrees.
  vtkSphereSource(int res=8);

  static vtkSphereSource *New() {return new vtkSphereSource;};
  const char *GetClassName() {return "vtkSphereSource";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set radius of sphere. Default is .5.
  vtkSetClampMacro(Radius,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(Radius,float);

  // Description:
  // Set the center of the sphere. Default is 0,0,0.
  vtkSetVector3Macro(Center,float);
  vtkGetVectorMacro(Center,float,3);

  // Description:
  // Set the number of points in the longitude direction (ranging from
  // StartTheta to EndTheta).
  vtkSetClampMacro(ThetaResolution,int,3,VTK_MAX_SPHERE_RESOLUTION);
  vtkGetMacro(ThetaResolution,int);

  // Description:
  // Set the number of points in the latitude direction (ranging
  // from StartPhi to EndPhi).
  vtkSetClampMacro(PhiResolution,int,2,VTK_MAX_SPHERE_RESOLUTION);
  vtkGetMacro(PhiResolution,int);

  // Description:
  // Set the starting longitude angle. By default StartTheta=0 degrees.
  vtkSetClampMacro(StartTheta,float,0.0,360.0);
  vtkGetMacro(StartTheta,float);

  // Description:
  // Set the ending longitude angle. By default EndTheta=360 degrees.
  vtkSetClampMacro(EndTheta,float,0.0,360.0);
  vtkGetMacro(EndTheta,float);

  // Description:
  // Set the starting latitude angle (0 is at north pole). By default
  // StartPhi=0 degrees.
  vtkSetClampMacro(StartPhi,float,0.0,360.0);
  vtkGetMacro(StartPhi,float);

  // Description:
  // Set the ending latitude angle. By default EndPhi=180 degrees.
  vtkSetClampMacro(EndPhi,float,0.0,360.0);
  vtkGetMacro(EndPhi,float);

protected:
  void Execute();
  float Radius;
  float Center[3];
  int ThetaResolution;
  int PhiResolution;
  float StartTheta;
  float EndTheta;
  float StartPhi;
  float EndPhi;

};

#endif


