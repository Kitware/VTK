/*=========================================================================

  Program:   Visualization Library
  Module:    ImpTC.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/

// .NAME vlImplicitTextureCoords - generate 1D, 2D, or 3D texture coordinates based on implicit function(s)
// .SECTION Description
// vlImplicitTextureCoords is a filter to generate 1D, 2D, or 3D texture 
// coordinates from one, two, or three implicit functions, respectively. 
// In combinations with a vlBooleanTexture map, the texture coordinates 
// can be used to highlight (via color or intensity) or cut (via 
// transparency) dataset geometry without any complex geometric processing. 
// (Note: the texture coordinates are refered to as r-s-t coordinates).
//    Note: use the transformation capabilities of vlImplicitFunction to
// orient, translate, and scale the implicit functions.

#ifndef __vlImplicitTextureCoords_h
#define __vlImplicitTextureCoords_h

#include "DS2DSF.hh"
#include "ImpFunc.hh"
#include "Trans.hh"

class vlImplicitTextureCoords : public vlDataSetToDataSetFilter 
{
public:
  vlImplicitTextureCoords();
  char *GetClassName() {return "vlImplicitTextureCoords";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Specify the dimension of the texture coordinates. Note: if the number of
  // implicit functions is less than the specified dimension, then the extra
  // coordinate values are set to zero.
  vlSetClampMacro(Dimension,int,1,3);
  vlGetMacro(Dimension,int);

  // Description:
  // Specify a quadric function to compute the r texture coordinate.
  vlSetObjectMacro(RFunction,vlImplicitFunction);
  vlGetObjectMacro(RFunction,vlImplicitFunction);

  // Description:
  // Specify a quadric function to compute the s texture coordinate.
  vlSetObjectMacro(SFunction,vlImplicitFunction);
  vlGetObjectMacro(SFunction,vlImplicitFunction);

  // Description:
  // Specify a quadric function to compute the t texture coordinate.
  vlSetObjectMacro(TFunction,vlImplicitFunction);
  vlGetObjectMacro(TFunction,vlImplicitFunction);

  // Description:
  // Specify a number to scale the implicit function.
  vlSetMacro(ScaleFactor,float);
  vlGetMacro(ScaleFactor,float);

  // Description:
  // Turn on/off texture coordinate clamping to range specified.
  vlSetMacro(Clamp,int);
  vlGetMacro(Clamp,int);
  vlBooleanMacro(Clamp,int);

  // Description:
  // Set r texture coordinate range,
  vlSetVector2Macro(RRange,float);
  vlGetVectorMacro(RRange,float,2);

  // Description:
  // Set s texture coordinate range,
  vlSetVector2Macro(SRange,float);
  vlGetVectorMacro(SRange,float,2);

  // Description:
  // Set t texture coordinate range,
  vlSetVector2Macro(TRange,float);
  vlGetVectorMacro(TRange,float,2);

protected:
  void Execute();

  int Dimension;

  vlImplicitFunction *RFunction;
  vlImplicitFunction *SFunction;
  vlImplicitFunction *TFunction;

  float ScaleFactor;

  int Clamp;
  float RRange[2];
  float SRange[2];
  float TRange[2];
};

#endif


