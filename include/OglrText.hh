/*=========================================================================

  Program:   Visualization Toolkit
  Module:    OglrText.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkOglrTexture - SGI OpenGL texture map
// .SECTION Description
// vtkOglrTexture is a concrete implementation of the abstract class 
// vtkTexture. vtkOglrTexture interfaces to the Silicon Graphics OpenGL 
// rendering library.

#ifndef __vtkOglrTexture_hh
#define __vtkOglrTexture_hh

#include "TextDev.hh"

class vtkOglrRenderer;

class vtkOglrTexture : public vtkTextureDevice
{
public:
  vtkOglrTexture();
  char *GetClassName() {return "vtkOglrTexture";};
  
  void Load(vtkTexture *txt, vtkRenderer *ren);
  void Load(vtkTexture *txt, vtkOglrRenderer *ren);
  
protected:
  vtkTimeStamp   LoadTime;
  long          Index;
  static   long GlobalIndex;
};

#endif
