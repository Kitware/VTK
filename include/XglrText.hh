/*=========================================================================

  Program:   Visualization Library
  Module:    XglrText.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlKXglrTexture - starbase texture map object
// .SECTION Description
// vlXglrTexture is a concrete implementation of the abstract class vlTexture.
// currently we don't support texture mapping on starbase.

#ifndef __vlXglrTexture_hh
#define __vlXglrTexture_hh

#include "TextDev.hh"

class vlXglrRenderer;

class vlXglrTexture : public vlTextureDevice
{
public:
  vlXglrTexture();
  char *GetClassName() {return "vlXglrTexture";};
  
  void Load(vlTexture *txt, vlRenderer *ren);
  void Load(vlTexture *txt, vlXglrRenderer *ren);
  
protected:
  vlTimeStamp   LoadTime;
  long          Index;
  static   long GlobalIndex;
};

#endif
