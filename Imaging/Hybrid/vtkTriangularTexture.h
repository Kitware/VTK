/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTriangularTexture.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTriangularTexture - generate 2D triangular texture map
// .SECTION Description
// vtkTriangularTexture is a filter that generates a 2D texture map based on
// the paper "Opacity-modulating Triangular Textures for Irregular Surfaces,"
// by Penny Rheingans, IEEE Visualization '96, pp. 219-225.
// The textures assume texture coordinates of (0,0), (1.0) and
// (.5, sqrt(3)/2). The sequence of texture values is the same along each
// edge of the triangular texture map. So, the assignment order of texture
// coordinates is arbitrary.

// .SECTION See Also
// vtkTriangularTCoords

#ifndef __vtkTriangularTexture_h
#define __vtkTriangularTexture_h

#include "vtkImagingHybridModule.h" // For export macro
#include "vtkImageAlgorithm.h"

class VTKIMAGINGHYBRID_EXPORT vtkTriangularTexture : public vtkImageAlgorithm
{
public:
  vtkTypeMacro(vtkTriangularTexture,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate object with XSize and YSize = 64; the texture pattern =1
  // (opaque at centroid); and the scale factor set to 1.0.
  static vtkTriangularTexture *New();

  // Description:
  // Set a Scale Factor.
  vtkSetMacro(ScaleFactor,double);
  vtkGetMacro(ScaleFactor,double);

  // Description:
  // Set the X texture map dimension. Default is 64.
  vtkSetMacro(XSize,int);
  vtkGetMacro(XSize,int);

  // Description:
  // Set the Y texture map dimension. Default is 64.
  vtkSetMacro(YSize,int);
  vtkGetMacro(YSize,int);

  // Description:
  // Set the texture pattern.
  //    1 = opaque at centroid (default)
  //    2 = opaque at vertices
  //    3 = opaque in rings around vertices
  vtkSetClampMacro(TexturePattern,int,1,3);
  vtkGetMacro(TexturePattern,int);

protected:
  vtkTriangularTexture();
  ~vtkTriangularTexture() {};

  virtual int RequestInformation (vtkInformation *, vtkInformationVector**, vtkInformationVector *);
  virtual void ExecuteDataWithInformation(vtkDataObject *data, vtkInformation *outInfo);

  int XSize;
  int YSize;
  double ScaleFactor;

  int TexturePattern;
private:
  vtkTriangularTexture(const vtkTriangularTexture&);  // Not implemented.
  void operator=(const vtkTriangularTexture&);  // Not implemented.
};

#endif


