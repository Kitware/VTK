/*=========================================================================

  Program:   Visualization Library
  Module:    Texture.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlTexture - handles properties associated with a texture map
// .SECTION Description
// vlTexture is an object that handles loading and binding of texture
// maps. It obtains its data from a StructuredPoints input. Multiple 
// actors using the same texture map should share the same vlTexture
// object.  This reduces the amount of memory being used. Currently
// only 2D etxture maps are supported event though the data pipeline
// supports 1,2, and 3D texture coordinates.
// .SECTION See Also
// See vlRenderer for definition of #define's.

#ifndef __vlTexture_hh
#define __vlTexture_hh

#include "Render.hh"
#include "Object.hh"
#include "StrPts.hh"

class vlRenderer;

class vlTexture : public vlObject
{
public:
  vlTexture();
  char *GetClassName() {return "vlTexture";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Renders a texture map. It first checks the MTimes to make sure
  // the texture maps Input is valid then it invokes the Load method.
  virtual void Render(vlRenderer *ren);

  // Description:
  // Abstract interface to renderer. Each concrete subclass of vlTexture
  // will load its data into graphics system in response to this method
  // invocation.
  virtual void Load(vlRenderer *ren) = 0;

  // Description:
  // Turn on/off the repetition of the texture map when the texture
  // coords extend beyond the [0,1] range.
  vlGetMacro(Repeat,int);
  vlSetMacro(Repeat,int);
  vlBooleanMacro(Repeat,int);

  // Description:
  // Turn on/off linear interpolation of the texture map when rendering.
  vlGetMacro(Interpolate,int);
  vlSetMacro(Interpolate,int);
  vlBooleanMacro(Interpolate,int);

  // Description:
  // Specify 2D or 3D texture map.
  vlSetObjectMacro(Input,vlStructuredPoints);
  vlGetObjectMacro(Input,vlStructuredPoints);

protected:
  int   Repeat;
  int   Interpolate;
  vlStructuredPoints *Input;
};

#endif
