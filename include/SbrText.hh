/*=========================================================================

  Program:   Visualization Library
  Module:    SbrText.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlKSbrTexture - starbase texture map object
// .SECTION Description
// vlSbrTexture is a concrete implementation of the abstract class vlTexture.
// currently we don't support texture mapping on starbase.

#ifndef __vlSbrTexture_hh
#define __vlSbrTexture_hh

#include "Texture.hh"

class vlSbrRenderer;

class vlSbrTexture : public vlTexture
{
public:
  vlSbrTexture();
  char *GetClassName() {return "vlSbrTexture";};
  
  void Load(vlRenderer *ren);
  void Load(vlSbrRenderer *ren);
  
protected:
  vlTimeStamp   LoadTime;
  long          Index;
  static   long GlobalIndex;
};

#endif
