/*=========================================================================

  Program:   Visualization Library
  Module:    Glyph3D.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlGlyph3D - copy oriented and scaled geometry to every input point
// .SECTION Description
// vlGlyph3D is a filter that copies a geometry representation (in 
// vlPolyData form) to every input point. The geometry may be oriented
// along the input vectors or normals, and it may be scaled according
// to scalar data or vector magnitude.

#ifndef __vlGlyph3D_h
#define __vlGlyph3D_h

#include "DS2PolyF.hh"

#define SCALE_BY_SCALAR 0
#define SCALE_BY_VECTOR 1
#define USE_VECTOR 0
#define USE_NORMAL 1

class vlGlyph3D : public vlDataSetToPolyFilter
{
public:
  vlGlyph3D();
  ~vlGlyph3D();
  char *GetClassName() {return "vlGlyph3D";};
  void PrintSelf(ostream& os, vlIndent indent);

  void Update();

  vlSetObjectMacro(Source,vlPolyData);
  vlGetObjectMacro(Source,vlPolyData);

  vlBooleanMacro(Scaling,int);
  vlSetMacro(Scaling,int);
  vlGetMacro(Scaling,int);

  vlSetMacro(ScaleMode,int);
  vlGetMacro(ScaleMode,int);
  void ScaleByScalar() {this->SetScaleMode(SCALE_BY_SCALAR);};
  void ScaleByVector() {this->SetScaleMode(SCALE_BY_VECTOR);};

  vlSetMacro(ScaleFactor,float);
  vlGetMacro(ScaleFactor,float);

  vlSetVector2Macro(Range,float);
  vlGetVectorMacro(Range,float);

  vlBooleanMacro(Orient,int);
  vlSetMacro(Orient,int);
  vlGetMacro(Orient,int);

  vlSetMacro(VectorMode,int);
  vlGetMacro(VectorMode,int);
  void UseVector() {this->SetVectorMode(USE_VECTOR);};
  void UseNormal() {this->SetVectorMode(USE_NORMAL);};

protected:
  void Execute();
  vlPolyData *Source; // Geometry to copy to each point
  int Scaling; // Determine whether scaling of geometry is performed
  int ScaleMode; // Scale by scalar value or vector magnitude
  float ScaleFactor; // Scale factor to use to scale geometry
  float Range[2]; // Range to use to perform scalar scaling
  int Orient; // boolean controls whether to "orient" data
  int VectorMode; // Orient/scale via normal or via vector data
};

#endif


