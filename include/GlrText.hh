/*=========================================================================

  Program:   Visualization Library
  Module:    GlrText.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlGlrTexture - SGI gl texture map
// .SECTION Description
// vlGlrTexture is a concrete implementation of the abstract class vlTexture.
// vlGlrTexture interfaces to the Silicon Graphics gl rendering library.

#ifndef __vlGlrTexture_hh
#define __vlGlrTexture_hh

#include "Texture.hh"
#include "gl.h"

class vlGlrRenderer;

class vlGlrTexture : public vlTexture
{
public:
  vlGlrTexture();
  char *GetClassName() {return "vlGlrTexture";};
  
  void Load(vlRenderer *ren);
  void Load(vlGlrRenderer *ren);
  
protected:
  vlTimeStamp   LoadTime;
  long          Index;
  static   long GlobalIndex;
};

#endif
