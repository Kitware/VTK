/*=========================================================================

  Program:   Visualization Library
  Module:    TMap2Box.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlTextureMapToBox - generate 3D texture coordinates by mapping points into bounding box
// .SECTION Description
// vlTextureMapToBox is a filter that generates 3D texture coordinates
// by mapping input dataset points onto a bounding box. The bounding box
// can either be user specified or generated automatically. If the box
// is generated automatically, all points will lie inside of it. If a
// point lies outside the bounding box (only for manual box 
// specification), its generated texture coordinate will be clamped
// into the r-s-t texture coordinate range.

#ifndef __vlTextureMapToBox_h
#define __vlTextureMapToBox_h

#include "DS2DSF.hh"

class vlTextureMapToBox : public vlDataSetToDataSetFilter 
{
public:
  vlTextureMapToBox();
  ~vlTextureMapToBox() {};
  char *GetClassName() {return "vlTextureMapToBox";};
  void PrintSelf(ostream& os, vlIndent indent);

  void SetBox(float *box);
  void SetBox(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax);
  vlGetVectorMacro(Box,float,6);

  // Description:
  // Specify r-coordinate range for texture r-s-t coordinate triplet.
  vlSetVector2Macro(RRange,float);
  vlGetVectorMacro(RRange,float,2);

  // Description:
  // Specify s-coordinate range for texture r-s-t coordinate triplet.
  vlSetVector2Macro(SRange,float);
  vlGetVectorMacro(SRange,float,2);

  // Description:
  // Specify t-coordinate range for texture r-s-t coordinate triplet.
  vlSetVector2Macro(TRange,float);
  vlGetVectorMacro(TRange,float,2);

  // Description:
  // Turn on/off automatic bounding box generation.
  vlSetMacro(AutomaticBoxGeneration,int);
  vlGetMacro(AutomaticBoxGeneration,int);
  vlBooleanMacro(AutomaticBoxGeneration,int);

protected:
  void Execute();
  float Box[6];
  float RRange[2];
  float SRange[2];
  float TRange[2];
  int AutomaticBoxGeneration;
};

#endif


