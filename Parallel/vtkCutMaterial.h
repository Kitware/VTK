/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCutMaterial.h
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
// .NAME vtkCutMaterial - Automatically computes the cut plane for a material array pair.
// .SECTION Description
// vtkCutMaterial computes a cut plane based on an up vector, center of the bounding box
// and the location of the maximum variable value.
//  These computed values are available so that they can be used to set the camera
// for the best view of the plane.


#ifndef __vtkCutMaterial_h
#define __vtkCutMaterial_h

#include "vtkDataSetToPolyDataFilter.h"

class vtkPlane;

class VTK_PARALLEL_EXPORT vtkCutMaterial : public vtkDataSetToPolyDataFilter
{
public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeRevisionMacro(vtkCutMaterial,vtkDataSetToPolyDataFilter);
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
  // For now, we just use the cell values.
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
