/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TMap2Pl.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkTextureMapToPlane - generate texture coordinates by mapping points to plane
// .SECTION Description
// vtkTextureMapToPlane is a filter that generates 2D texture coordinates
// by mapping input dataset points onto a plane. The plane can either be
// user specified or generated automatically. (A least squares method is
// used to generate the plane).

#ifndef __vtkTextureMapToPlane_h
#define __vtkTextureMapToPlane_h

#include "DS2DSF.hh"

class vtkTextureMapToPlane : public vtkDataSetToDataSetFilter 
{
public:
  vtkTextureMapToPlane();
  ~vtkTextureMapToPlane() {};
  char *GetClassName() {return "vtkTextureMapToPlane";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify plane normal.
  vtkSetVector3Macro(Normal,float);
  vtkGetVectorMacro(Normal,float,3);

  // Description:
  // Specify s-coordinate range for texture s-t coordinate pair.
  vtkSetVector2Macro(SRange,float);
  vtkGetVectorMacro(SRange,float,2);

  // Description:
  // Specify t-coordinate range for texture s-t coordinate pair.
  vtkSetVector2Macro(TRange,float);
  vtkGetVectorMacro(TRange,float,2);

  // Description:
  // Turn on/off automatic plane generation.
  vtkSetMacro(AutomaticPlaneGeneration,int);
  vtkGetMacro(AutomaticPlaneGeneration,int);
  vtkBooleanMacro(AutomaticPlaneGeneration,int);

protected:
  void Execute();
  void ComputeNormal();
  float Normal[3];
  float SRange[2];
  float TRange[2];
  int AutomaticPlaneGeneration;
};

#endif


