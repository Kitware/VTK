/*=========================================================================

  Program:   Visualization Library
  Module:    BoolText.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlBooleanTexture - generate 2D texture map based on combinations of inside, outside, and on region boundary
// .SECTION Description
// vlBooleanTexture is a filter to generate a 2D texture map based on 
// combinations of inside, outside, and on region boundary. The "region" is
// implicitly represented via 2D texture coordinates. These texture 
// coordinates are normally generated using a filter like 
// vlImplicitTextureCoords which generates the texture coordinates for 
// any implicit function.
//   vlBooleanTexture generates the map according to the s-t texture
// coordinates plus the notion of being in, on, or outside of a
// region. An in region is when the texture coordinate is between
// (0,0.5-thickness/2).  An out region is where the texture coordinate
// is (0.5+thickness/2). An on region is between
// (0.5-thickness/2,0.5+thickness/2). The combination in, on, and out
// for each of the s-t texture coordinates results in 8 possible
// combinations. For each combination, a different value of intensity
// and transparency can be assigned. To assign maximum intensity
// and/or opacity use the value 255. A minimum value of 0 results in
// a black region (for intensity) and a fully transparent region (for
// transparency).
// .SECTION See Also
// vlImplicitTextureCoords

#ifndef __vlBooleanTexture_h
#define __vlBooleanTexture_h

#include "SPtsSrc.hh"

class vlBooleanTexture : public vlStructuredPointsSource
{
public:
  vlBooleanTexture();
  ~vlBooleanTexture() {};
  char *GetClassName() {return "vlBooleanTexture";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Set the X texture map dimension.
  vlSetMacro(XSize,int);
  vlGetMacro(XSize,int);

  // Description:
  // Set the Y texture map dimension.
  vlSetMacro(YSize,int);
  vlGetMacro(YSize,int);

  // Description:
  // Set the thickness of the "on" region.
  vlSetMacro(Thickness,int);
  vlGetMacro(Thickness,int);

  // Description:
  // Specify intensity/transparency for "in/in" region.
  vlSetVector2Macro(InIn,unsigned char);
  vlGetVectorMacro(InIn,unsigned char,2);

  // Description:
  // Specify intensity/transparency for "in/out" region.
  vlSetVector2Macro(InOut,unsigned char);
  vlGetVectorMacro(InOut,unsigned char,2);

  // Description:
  // Specify intensity/transparency for "out/in" region.
  vlSetVector2Macro(OutIn,unsigned char);
  vlGetVectorMacro(OutIn,unsigned char,2);

  // Description:
  // Specify intensity/transparency for "out/out" region.
  vlSetVector2Macro(OutOut,unsigned char);
  vlGetVectorMacro(OutOut,unsigned char,2);

  // Description:
  // Specify intensity/transparency for "on/on" region.
  vlSetVector2Macro(OnOn,unsigned char);
  vlGetVectorMacro(OnOn,unsigned char,2);

  // Description:
  // Specify intensity/transparency for "on/in" region.
  vlSetVector2Macro(OnIn,unsigned char);
  vlGetVectorMacro(OnIn,unsigned char,2);

  // Description:
  // Specify intensity/transparency for "on/out" region.
  vlSetVector2Macro(OnOut,unsigned char);
  vlGetVectorMacro(OnOut,unsigned char,2);

  // Description:
  // Specify intensity/transparency for "in/on" region.
  vlSetVector2Macro(InOn,unsigned char);
  vlGetVectorMacro(InOn,unsigned char,2);

  // Description:
  // Specify intensity/transparency for "out/on" region.
  vlSetVector2Macro(OutOn,unsigned char);
  vlGetVectorMacro(OutOn,unsigned char,2);

protected:
  void Execute();

  int XSize;
  int YSize;

  int Thickness;
  unsigned char InIn[2];
  unsigned char InOut[2];
  unsigned char OutIn[2];
  unsigned char OutOut[2];
  unsigned char OnOn[2];
  unsigned char OnIn[2];
  unsigned char OnOut[2];
  unsigned char InOn[2];
  unsigned char OutOn[2];

};

#endif


