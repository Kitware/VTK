/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SbrText.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkKSbrTexture - starbase texture map object
// .SECTION Description
// vtkSbrTexture is a concrete implementation of the abstract class vtkTexture.
// currently we don't support texture mapping on starbase.

#ifndef __vtkSbrTexture_hh
#define __vtkSbrTexture_hh

#include "Texture.hh"

class vtkSbrRenderer;

class vtkSbrTexture : public vtkTexture
{
public:
  vtkSbrTexture();
  char *GetClassName() {return "vtkSbrTexture";};
  
  void Load(vtkTexture *txt, vtkRenderer *ren);
  void Load(vtkTexture *txt, vtkSbrRenderer *ren);
  
protected:
  vtkTimeStamp   LoadTime;
  long          Index;
  static   long GlobalIndex;
};

#endif
