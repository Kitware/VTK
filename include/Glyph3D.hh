/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Glyph3D.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkGlyph3D - copy oriented and scaled geometry to every input point
// .SECTION Description
// vtkGlyph3D is a filter that copies a geometric representation (specified
// as polygonal data) to every input point. The geometry may be oriented
// along the input vectors or normals, and it may be scaled according
// to scalar data or vector magnitude. The geometry is supplied via the 
// Source instance variable; the points come from the Input.
// .SECTION See Also
// vtkTensorEllipsoid

#ifndef __vtkGlyph3D_h
#define __vtkGlyph3D_h

#include "DS2PolyF.hh"

#define SCALE_BY_SCALAR 0
#define SCALE_BY_VECTOR 1
#define USE_VECTOR 0
#define USE_NORMAL 1

class vtkGlyph3D : public vtkDataSetToPolyFilter
{
public:
  vtkGlyph3D();
  ~vtkGlyph3D();
  char *GetClassName() {return "vtkGlyph3D";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void Update();

  // Description:
  // Specify the geometry to copy to each point.
  vtkSetObjectMacro(Source,vtkPolyData);
  vtkGetObjectMacro(Source,vtkPolyData);

  // Description:
  // Turn on/off scaling of input geometry.
  vtkSetMacro(Scaling,int);
  vtkBooleanMacro(Scaling,int);
  vtkGetMacro(Scaling,int);

  // Description:
  // Either scale by scalar or by vector/normal magnitude.
  vtkSetMacro(ScaleMode,int);
  vtkGetMacro(ScaleMode,int);
  void ScaleByScalar() {this->SetScaleMode(SCALE_BY_SCALAR);};
  void ScaleByVector() {this->SetScaleMode(SCALE_BY_VECTOR);};

  // Description:
  // Specify scale factor to scale object by.
  vtkSetMacro(ScaleFactor,float);
  vtkGetMacro(ScaleFactor,float);

  // Description:
  // Specify range to map scalar values into.
  vtkSetVector2Macro(Range,float);
  vtkGetVectorMacro(Range,float,2);

  // Description:
  // Turn on/off orienting of input geometry along vector/normal.
  vtkSetMacro(Orient,int);
  vtkBooleanMacro(Orient,int);
  vtkGetMacro(Orient,int);

  // Description:
  // Specify whether to use vector or normal to perform vector operations.
  vtkSetMacro(VectorMode,int);
  vtkGetMacro(VectorMode,int);
  void UseVector() {this->SetVectorMode(USE_VECTOR);};
  void UseNormal() {this->SetVectorMode(USE_NORMAL);};

protected:
  void Execute();
  vtkPolyData *Source; // Geometry to copy to each point
  int Scaling; // Determine whether scaling of geometry is performed
  int ScaleMode; // Scale by scalar value or vector magnitude
  float ScaleFactor; // Scale factor to use to scale geometry
  float Range[2]; // Range to use to perform scalar scaling
  int Orient; // boolean controls whether to "orient" data
  int VectorMode; // Orient/scale via normal or via vector data
};

#endif


