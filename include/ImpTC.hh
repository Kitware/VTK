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
//    The texture coordinates are automatically normalized to lie between (0,1). 
// Thus, no matter what the implicit functions evaluate to, the resulting texture 
// coordinates lie between (0,1), with the zero implicit function value mapped 
// to the 0.5 texture coordinates value. Depending upon the maximum 
// negative/positive implicit function values, the full (0,1) range may not be 
// occupied (i.e., the positive/negative ranges are mapped using the same scale 
// factor).
// .SECTION Caveats
// You can use the transformation capabilities of vtkImplicitFunction to
// orient, translate, and scale the implicit functions. Also, the dimension of 
// the texture coordinates is implicitly defined by the number of implicit 
// functions defined.

#ifndef __vtkImplicitTextureCoords_h
#define __vtkImplicitTextureCoords_h

#include "DS2DSF.hh"
#include "ImpFunc.hh"

class vtkImplicitTextureCoords : public vtkDataSetToDataSetFilter 
{
public:
  vtkImplicitTextureCoords();
  char *GetClassName() {return "vtkImplicitTextureCoords";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify an implicit function to compute the r texture coordinate.
  vtkSetObjectMacro(RFunction,vtkImplicitFunction);
  vtkGetObjectMacro(RFunction,vtkImplicitFunction);

  // Description:
  // Specify a implicit function to compute the s texture coordinate.
  vtkSetObjectMacro(SFunction,vtkImplicitFunction);
  vtkGetObjectMacro(SFunction,vtkImplicitFunction);

  // Description:
  // Specify a implicit function to compute the t texture coordinate.
  vtkSetObjectMacro(TFunction,vtkImplicitFunction);
  vtkGetObjectMacro(TFunction,vtkImplicitFunction);

protected:
  void Execute();

  vtkImplicitFunction *RFunction;
  vtkImplicitFunction *SFunction;
  vtkImplicitFunction *TFunction;
};

#endif


