/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TMap2Box.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkTextureMapToBox - generate 3D texture coordinates by mapping points into bounding box
// .SECTION Description
// vtkTextureMapToBox is a filter that generates 3D texture coordinates
// by mapping input dataset points onto a bounding box. The bounding box
// can either be user specified or generated automatically. If the box
// is generated automatically, all points will lie inside of it. If a
// point lies outside the bounding box (only for manual box 
// specification), its generated texture coordinate will be clamped
// into the r-s-t texture coordinate range.

#ifndef __vtkTextureMapToBox_h
#define __vtkTextureMapToBox_h

#include "DS2DSF.hh"

class vtkTextureMapToBox : public vtkDataSetToDataSetFilter 
{
public:
  vtkTextureMapToBox();
  ~vtkTextureMapToBox() {};
  char *GetClassName() {return "vtkTextureMapToBox";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetBox(float *box);
  void SetBox(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax);
  vtkGetVectorMacro(Box,float,6);

  // Description:
  // Specify r-coordinate range for texture r-s-t coordinate triplet.
  vtkSetVector2Macro(RRange,float);
  vtkGetVectorMacro(RRange,float,2);

  // Description:
  // Specify s-coordinate range for texture r-s-t coordinate triplet.
  vtkSetVector2Macro(SRange,float);
  vtkGetVectorMacro(SRange,float,2);

  // Description:
  // Specify t-coordinate range for texture r-s-t coordinate triplet.
  vtkSetVector2Macro(TRange,float);
  vtkGetVectorMacro(TRange,float,2);

  // Description:
  // Turn on/off automatic bounding box generation.
  vtkSetMacro(AutomaticBoxGeneration,int);
  vtkGetMacro(AutomaticBoxGeneration,int);
  vtkBooleanMacro(AutomaticBoxGeneration,int);

protected:
  void Execute();
  float Box[6];
  float RRange[2];
  float SRange[2];
  float TRange[2];
  int AutomaticBoxGeneration;
};

#endif


