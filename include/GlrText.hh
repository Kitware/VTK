/*=========================================================================

  Program:   Visualization Toolkit
  Module:    GlrText.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkGlrTexture - SGI gl texture map
// .SECTION Description
// vtkGlrTexture is a concrete implementation of the abstract class vtkTexture.
// vtkGlrTexture interfaces to the Silicon Graphics gl rendering library.

#ifndef __vtkGlrTexture_hh
#define __vtkGlrTexture_hh

#include "TextDev.hh"

class vtkGlrRenderer;

class vtkGlrTexture : public vtkTextureDevice
{
public:
  vtkGlrTexture();
  char *GetClassName() {return "vtkGlrTexture";};
  
  void Load(vtkTexture *txt, vtkRenderer *ren);
  void Load(vtkTexture *txt, vtkGlrRenderer *ren);
  
protected:
  vtkTimeStamp   LoadTime;
  long          Index;
  static   long GlobalIndex;
};

#endif
