/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMassProperties.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

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

class VTK_GRAPHICS_EXPORT vtkMassProperties : public vtkProcessObject
{
public:
  // Description:
  // Constructs with initial values of zero.
  static vtkMassProperties *New();

  vtkTypeRevisionMacro(vtkMassProperties,vtkProcessObject);
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

private:
  vtkMassProperties(const vtkMassProperties&);  // Not implemented.
  void operator=(const vtkMassProperties&);  // Not implemented.
};

#endif


