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
/**
 * @class   vtkTriangularTCoords
 * @brief   2D texture coordinates based for triangles.
 *
 * vtkTriangularTCoords is a filter that generates texture coordinates
 * for triangles. Texture coordinates for each triangle are:
 * (0,0), (1,0) and (.5,sqrt(3)/2). This filter assumes that the triangle
 * texture map is symmetric about the center of the triangle. Thus the order
 * Of the texture coordinates is not important. The procedural texture
 * in vtkTriangularTexture is designed with this symmetry. For more information
 * see the paper "Opacity-modulating Triangular Textures for Irregular
 * Surfaces,"  by Penny Rheingans, IEEE Visualization '96, pp. 219-225.
 * @sa
 * vtkTriangularTexture vtkThresholdPoints vtkTextureMapToPlane
 * vtkTextureMapToSphere vtkTextureMapToCylinder
*/

#ifndef vtkTriangularTCoords_h
#define vtkTriangularTCoords_h

#include "vtkFiltersTextureModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSTEXTURE_EXPORT vtkTriangularTCoords : public vtkPolyDataAlgorithm
{
public:
  static vtkTriangularTCoords *New();
  vtkTypeMacro(vtkTriangularTCoords,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkTriangularTCoords() {}
  ~vtkTriangularTCoords() {}

  // Usual data generation method
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
private:
  vtkTriangularTCoords(const vtkTriangularTCoords&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTriangularTCoords&) VTK_DELETE_FUNCTION;
};

#endif
