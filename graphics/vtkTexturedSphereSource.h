/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTexturedSphereSource.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkTexturedSphereSource - create a sphere centered at the origin
// .SECTION Description
// vtkTexturedSphereSource creates a polygonal sphere of specified radius 
// centered at the origin. The resolution (polygonal discretization) in both 
// the latitude (phi) and longitude (theta) directions can be specified. 
// It also is possible to create partial sphere by specifying maximum phi and 
// theta angles.

#ifndef __vtkTexturedSphereSource_h
#define __vtkTexturedSphereSource_h

#include "vtkPolySource.h"

#define VTK_MAX_SPHERE_RESOLUTION 1024

class VTK_EXPORT vtkTexturedSphereSource : public vtkPolySource 
{
public:
  vtkTexturedSphereSource(int res=8);
  static vtkTexturedSphereSource *New() {return new vtkTexturedSphereSource;};
  char *GetClassName() {return "vtkTexturedSphereSource";};
  void PrintSelf(ostream& os, vtkIndent indent);

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
  void Execute();
  float Radius;
  float Theta;
  float Phi;
  int ThetaResolution;
  int PhiResolution;

};

#endif


