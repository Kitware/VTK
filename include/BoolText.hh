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
// .SECTION See Also
// vlImplicitTextureCoords

#ifndef __vlBooleanTexture_h
#define __vlBooleanTexture_h

#include "Object.hh"

class vlBooleanTexture : public vlObject //needs base class from texture object
{
public:
  vlBooleanTexture();
  char *GetClassName() {return "vlBooleanTexture";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlSetMacro(Thickness,int);
  vlGetMacro(Thickness,int);

protected:
  int Thickness;
};

#endif


