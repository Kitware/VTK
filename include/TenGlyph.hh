/*=========================================================================

  Program:   Visualization Library
  Module:    TenGlyph.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkTensorGlyph - scale and orient glyph according to tensor eigenvalues and eigenvectors
// .SECTION Description
// vtkTensorGlyph is a filter that copies a geometric representation (specified
// as polygonal data) to every input point. The geometric representation, or 
// glyph, can be scaled and/or rotated according to the tensor at the input 
// point. Scaling and rotation is controlled by the eigenvalues/eigenvectors
// of the tensor as follows. For each tensor, the eigenvalues (and associated
// eigenvectors) are sorted to determine the major, medium, and minor 
// eigenvalues/eigenvectors. The major eigenvalue scales the glyph in the 
// x-direction, the medium in the y-direction, and the minor in the 
// z-direction. Then, the glyph is rotated so that the glyph's local x-axis 
// lies along the major eigenvector, y-axis along the medium eigenvector, and
// z-axis along the minor.
//    A scale factor is provided to control the amount of scaling. Also, you 
// can turn off scaling completely if desired.
//    Another instance variable, ExtractEigenvalues, has been provided to 
// control extraction of eigenvalues/eigenvectors. If this boolean is false,
// then eigenvalues/eigenvectors are not extracted, and the columns of the
// matrix are taken as the eigenvectors (norm of column is eigenvalue). 
// This allows additional capability over the vtkGlyph3D object. That is, the
// glyph can be oriented in three directions instead of one.
// .SECTION See Also
// vtkGlyph3D

#ifndef __vtkTensorGlyph_h
#define __vtkTensorGlyph_h

#include "DS2PolyF.hh"

class vtkTensorGlyph : public vtkDataSetToPolyFilter
{
public:
  vtkTensorGlyph();
  ~vtkTensorGlyph();
  char *GetClassName() {return "vtkTensorGlyph";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void Update();

  // Description:
  // Specify the geometry to copy to each point.
  vtkSetObjectMacro(Source,vtkPolyData);
  vtkGetObjectMacro(Source,vtkPolyData);

  // Description:
  // Turn on/off scaling of glyph with eigenvalues.
  vtkSetMacro(Scaling,int);
  vtkBooleanMacro(Scaling,int);
  vtkGetMacro(Scaling,int);

  // Description:
  // Specify scale factor to scale object by. (Scale factor always affects
  // output even if scaling is off).
  vtkSetMacro(ScaleFactor,float);
  vtkGetMacro(ScaleFactor,float);

  // Description:
  // Turn on/off extraction of eigenvalues from tensor.
  vtkSetMacro(ExtractEigenvalues,int);
  vtkBooleanMacro(ExtractEigenvalues,int);
  vtkGetMacro(ExtractEigenvalues,int);

protected:
  void Execute();

  vtkPolyData *Source; // Geometry to copy to each point
  int Scaling; // Determine whether scaling of geometry is performed
  float ScaleFactor; // Scale factor to use to scale geometry
  int ExtractEigenvalues; // Boolean controls eigenfunction extraction
};

#endif
