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
//
// Copy PolyData at every input point.
//
#ifndef __vlGlyph3D_h
#define __vlGlyph3D_h

#include "DS2PolyF.hh"

#define SCALE_BY_SCALAR 0
#define SCALE_BY_VECTOR 1
#define ORIENT_BY_VECTOR 0
#define ORIENT_BY_NORMAL 1

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

  vlSetMacro(OrientMode,int);
  vlGetMacro(OrientMode,int);
  void OrientByVector() {this->SetScaleMode(ORIENT_BY_VECTOR);};
  void OrientByNormal() {this->SetScaleMode(ORIENT_BY_NORMAL);};

protected:
  // Usual data generation method
  void Execute();
  // Geometry to copy to each point
  vlPolyData *Source;
  // Determine whether scaling of geometry is performed
  int Scaling;
  // Scale by scalar value or vector magnitude
  int ScaleMode;
  // Scale factor to use to scale geometry
  float ScaleFactor;
  // Range to use to perform scalar scaling
  float Range[2];
  // boolean controls whether to "orient" data
  int Orient;
  // Orient via normal or via vector data
  int OrientMode;
};

#endif


