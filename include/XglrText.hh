/*=========================================================================

  Program:   Visualization Toolkit
  Module:    XglrText.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkKXglrTexture - starbase texture map object
// .SECTION Description
// vtkXglrTexture is a concrete implementation of the abstract class vtkTexture.
// currently we don't support texture mapping on starbase.

#ifndef __vtkXglrTexture_hh
#define __vtkXglrTexture_hh

#include "TextDev.hh"

class vtkXglrRenderer;

class vtkXglrTexture : public vtkTextureDevice
{
public:
  vtkXglrTexture();
  char *GetClassName() {return "vtkXglrTexture";};
  
  void Load(vtkTexture *txt, vtkRenderer *ren);
  void Load(vtkTexture *txt, vtkXglrRenderer *ren);
  
protected:
  vtkTimeStamp   LoadTime;
  long          Index;
  static   long GlobalIndex;
};

#endif
