/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCutMaterial.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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


#ifndef vtkCutMaterial_h
#define vtkCutMaterial_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkPlane;

class VTKFILTERSPARALLEL_EXPORT vtkCutMaterial : public vtkPolyDataAlgorithm
{
public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeMacro(vtkCutMaterial,vtkPolyDataAlgorithm);
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
  vtkSetVector3Macro(UpVector, double);
  vtkGetVector3Macro(UpVector, double);

  // Description:
  // Accesses to the values computed during the execute method.  They
  // could be used to get a good camera view for the resulting plane.
  vtkGetVector3Macro(MaximumPoint, double);
  vtkGetVector3Macro(CenterPoint, double);
  vtkGetVector3Macro(Normal, double);

protected:
  vtkCutMaterial();
  ~vtkCutMaterial();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *); //generate output data
  virtual int FillInputPortInformation(int port, vtkInformation *info);
  void ComputeMaximumPoint(vtkDataSet *input);
  void ComputeNormal();

  char *MaterialArrayName;
  int Material;
  char *ArrayName;
  double UpVector[3];
  double MaximumPoint[3];
  double CenterPoint[3];
  double Normal[3];

  vtkPlane *PlaneFunction;

private:
  vtkCutMaterial(const vtkCutMaterial&);  // Not implemented.
  void operator=(const vtkCutMaterial&);  // Not implemented.
};

#endif
