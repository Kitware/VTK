/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCubeSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCubeSource - create a polygonal representation of a cube
// .SECTION Description
// vtkCubeSource creates a cube centered at origin. The cube is represented
// with four-sided polygons. It is possible to specify the length, width,
// and height of the cube independently.

#ifndef __vtkCubeSource_h
#define __vtkCubeSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSSOURCES_EXPORT vtkCubeSource : public vtkPolyDataAlgorithm
{
public:
  static vtkCubeSource *New();
  vtkTypeMacro(vtkCubeSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the length of the cube in the x-direction.
  vtkSetClampMacro(XLength,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(XLength,double);

  // Description:
  // Set the length of the cube in the y-direction.
  vtkSetClampMacro(YLength,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(YLength,double);

  // Description:
  // Set the length of the cube in the z-direction.
  vtkSetClampMacro(ZLength,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(ZLength,double);

  // Description:
  // Set the center of the cube.
  vtkSetVector3Macro(Center,double);
  vtkGetVectorMacro(Center,double,3);

  // Description:
  // Convenience method allows creation of cube by specifying bounding box.
  void SetBounds(double xMin, double xMax,
                 double yMin, double yMax,
                 double zMin, double zMax);
  void SetBounds(double bounds[6]);

protected:
  vtkCubeSource(double xL=1.0, double yL=1.0, double zL=1.0);
  ~vtkCubeSource() {};

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  double XLength;
  double YLength;
  double ZLength;
  double Center[3];
private:
  vtkCubeSource(const vtkCubeSource&);  // Not implemented.
  void operator=(const vtkCubeSource&);  // Not implemented.
};

#endif
