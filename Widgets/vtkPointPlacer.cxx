/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointPlacer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointPlacer.h"

#include "vtkRenderer.h"

vtkCxxRevisionMacro(vtkPointPlacer, "1.3");

//----------------------------------------------------------------------
vtkPointPlacer::vtkPointPlacer()
{
  this->PixelTolerance           = 5;
  this->WorldTolerance           = 0.001;
}

//----------------------------------------------------------------------
vtkPointPlacer::~vtkPointPlacer()
{
}

//----------------------------------------------------------------------
int vtkPointPlacer::UpdateWorldPosition( vtkRenderer *vtkNotUsed(ren),
                                         double *vtkNotUsed(worldPos),
                                         double *vtkNotUsed(worldOrient) )
{
  return 1;
}

//----------------------------------------------------------------------
void vtkPointPlacer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);  
}
