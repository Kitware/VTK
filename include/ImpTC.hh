/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImpTC.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/

// .NAME vtkImplicitTextureCoords - generate 1D, 2D, or 3D texture coordinates based on implicit function(s)
// .SECTION Description
// vtkImplicitTextureCoords is a filter to generate 1D, 2D, or 3D texture 
// coordinates from one, two, or three implicit functions, respectively. 
// In combinations with a vtkBooleanTexture map, the texture coordinates 
// can be used to highlight (via color or intensity) or cut (via 
// transparency) dataset geometry without any complex geometric processing. 
// (Note: the texture coordinates are refered to as r-s-t coordinates).
//    Note: use the transformation capabilities of vtkImplicitFunction to
// orient, translate, and scale the implicit functions.

#ifndef __vtkImplicitTextureCoords_h
#define __vtkImplicitTextureCoords_h

#include "DS2DSF.hh"
#include "ImpFunc.hh"
#include "Trans.hh"

class vtkImplicitTextureCoords : public vtkDataSetToDataSetFilter 
{
public:
  vtkImplicitTextureCoords();
  char *GetClassName() {return "vtkImplicitTextureCoords";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the dimension of the texture coordinates. Note: if the number of
  // implicit functions is less than the specified dimension, then the extra
  // coordinate values are set to zero.
  vtkSetClampMacro(Dimension,int,1,3);
  vtkGetMacro(Dimension,int);

  // Description:
  // Specify a quadric function to compute the r texture coordinate.
  vtkSetObjectMacro(RFunction,vtkImplicitFunction);
  vtkGetObjectMacro(RFunction,vtkImplicitFunction);

  // Description:
  // Specify a quadric function to compute the s texture coordinate.
  vtkSetObjectMacro(SFunction,vtkImplicitFunction);
  vtkGetObjectMacro(SFunction,vtkImplicitFunction);

  // Description:
  // Specify a quadric function to compute the t texture coordinate.
  vtkSetObjectMacro(TFunction,vtkImplicitFunction);
  vtkGetObjectMacro(TFunction,vtkImplicitFunction);

  // Description:
  // Specify a number to scale the implicit function.
  vtkSetMacro(ScaleFactor,float);
  vtkGetMacro(ScaleFactor,float);

  // Description:
  // Turn on/off texture coordinate clamping to range specified.
  vtkSetMacro(Clamp,int);
  vtkGetMacro(Clamp,int);
  vtkBooleanMacro(Clamp,int);

  // Description:
  // Set r texture coordinate range,
  vtkSetVector2Macro(RRange,float);
  vtkGetVectorMacro(RRange,float,2);

  // Description:
  // Set s texture coordinate range,
  vtkSetVector2Macro(SRange,float);
  vtkGetVectorMacro(SRange,float,2);

  // Description:
  // Set t texture coordinate range,
  vtkSetVector2Macro(TRange,float);
  vtkGetVectorMacro(TRange,float,2);

protected:
  void Execute();

  int Dimension;

  vtkImplicitFunction *RFunction;
  vtkImplicitFunction *SFunction;
  vtkImplicitFunction *TFunction;

  float ScaleFactor;

  int Clamp;
  float RRange[2];
  float SRange[2];
  float TRange[2];
};

#endif


