/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTriangularTCoords.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTriangularTCoords - 2D texture coordinates based for triangles.
// .SECTION Description
// vtkTriangularTCoords is a filter that generates texture coordinates
// for triangles. Texture coordinates for each triangle are:
// (0,0), (1,0) and (.5,sqrt(3)/2). This filter assumes that the triangle
// texture map is symmetric about the center of the triangle. Thus the order
// Of the texture coordinates is not important. The procedural texture
// in vtkTriangularTexture is designed with this symmetry. For more information
// see the paper "Opacity-modulating Triangular Textures for Irregular 
// Surfaces,"  by Penny Rheingans, IEEE Visualization '96, pp. 219-225.
// .SECTION See Also
// vtkTriangularTexture vtkThresholdPoints vtkTextureMapToPlane 
// vtkTextureMapToSphere vtkTextureMapToCylinder 

#ifndef __vtkTriangularTCoords_h
#define __vtkTriangularTCoords_h

#include "vtkPolyDataAlgorithm.h"

class VTK_GRAPHICS_EXPORT vtkTriangularTCoords : public vtkPolyDataAlgorithm
{
public:
  static vtkTriangularTCoords *New();
  vtkTypeMacro(vtkTriangularTCoords,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkTriangularTCoords() {};
  ~vtkTriangularTCoords() {};

  // Usual data generation method
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
private:
  vtkTriangularTCoords(const vtkTriangularTCoords&);  // Not implemented.
  void operator=(const vtkTriangularTCoords&);  // Not implemented.
};

#endif
