/*=========================================================================

  Program:   Visualization Library
  Module:    Cursor3D.hh
  Language:  C++
  Date:      5/16/94
  Version:   1.1

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Create a 3D cursor
//
#ifndef __vlCursor3D_h
#define __vlCursor3D_h

#include "PolySrc.hh"

class vlCursor3D : public vlPolySource 
{
public:
  vlCursor3D();
  char *GetClassName() {return "vlCursor3D";};
  void PrintSelf(ostream& os, vlIndent indent);

  void SetModelBounds(float *bounds);
  void SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax);
  vlGetVectorMacro(ModelBounds,float);

  vlSetVector3Macro(FocalPoint,float);
  vlGetVectorMacro(FocalPoint,float);

  vlSetMacro(Outline,int);
  vlGetMacro(Outline,int);
  vlBooleanMacro(Outline,int);

  vlSetMacro(Axes,int);
  vlGetMacro(Axes,int);
  vlBooleanMacro(Axes,int);

  vlSetMacro(XShadows,int);
  vlGetMacro(XShadows,int);
  vlBooleanMacro(XShadows,int);

  vlSetMacro(YShadows,int);
  vlGetMacro(YShadows,int);
  vlBooleanMacro(YShadows,int);

  vlSetMacro(ZShadows,int);
  vlGetMacro(ZShadows,int);
  vlBooleanMacro(ZShadows,int);

  vlSetMacro(Wrap,int);
  vlGetMacro(Wrap,int);
  vlBooleanMacro(Wrap,int);

protected:
  void Execute();

  float ModelBounds[6];
  float FocalPoint[3];
  int Outline;
  int Axes;
  int XShadows;
  int YShadows;
  int ZShadows;
  int Wrap;
};

#endif


