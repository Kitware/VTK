/*=========================================================================

  Program:   Visualization Library
  Module:    TMap2Pl.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlTextureMapToPlane - generate texture coordinates by mapping points to plane
// .SECTION Description
// vlTextureMapToPlane is a filter that generates 2D texture coordinates
// by mapping input dataset points onto a plane. The plane can either be
// user specified or generated automatically. (A least squares method is
// used to generate the plane).

#ifndef __vlTextureMapToPlane_h
#define __vlTextureMapToPlane_h

#include "DS2DSF.hh"

class vlTextureMapToPlane : public vlDataSetToDataSetFilter 
{
public:
  vlTextureMapToPlane();
  ~vlTextureMapToPlane() {};
  char *GetClassName() {return "vlTextureMapToPlane";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Specify plane normal.
  vlSetVector3Macro(Normal,float);
  vlGetVectorMacro(Normal,float,3);

  // Description:
  // Specify s-coordinate range for texture s-t coordinate pair.
  vlSetVector2Macro(SRange,float);
  vlGetVectorMacro(SRange,float,2);

  // Description:
  // Specify t-coordinate range for texture s-t coordinate pair.
  vlSetVector2Macro(TRange,float);
  vlGetVectorMacro(TRange,float,2);

  // Description:
  // Turn on/off automatic plane generation.
  vlSetMacro(AutomaticPlaneGeneration,int);
  vlGetMacro(AutomaticPlaneGeneration,int);
  vlBooleanMacro(AutomaticPlaneGeneration,int);

protected:
  void Execute();
  void ComputeNormal();
  float Normal[3];
  float SRange[2];
  float TRange[2];
  int AutomaticPlaneGeneration;
};

#endif


